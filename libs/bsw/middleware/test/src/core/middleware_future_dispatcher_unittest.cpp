#include <cstdint>
#include <iostream>

#include <etl/array.h>
#include <etl/delegate.h>
#include <etl/expected.h>
#include <etl/type_traits.h>
#include <etl/utility.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dsl_logger.h"
#include "middleware/core/future.h"
#include "middleware/core/future_dispatcher_type_selector.h"
#include "middleware/core/ifuture.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "mock/system_timer_provider_mock.h"

namespace middleware::core::test
{

template <uint32_t TIMEOUT, uint16_t LIMIT>
struct TestConfig
{
    using FutureUnderTest = Future<FutureTraits<int, 0U, false, TIMEOUT>>;
    static constexpr uint16_t REQUEST_LIMIT = LIMIT;
};

template <typename Config>
class FutureDispatcherTestSuite
    : public ::testing::Test,
      public FutureDispatcherTypeSelector<typename Config::FutureUnderTest::Traits, Config::REQUEST_LIMIT>::Type
{
  public:
    class RefApp
    {
      public:
        MOCK_METHOD(void,
                    receiveFutureData,
                    ((etl::expected<typename Config::FutureUnderTest::Type, IFuture::State>)&&),
                    ());
    };

    void SetUp() override
    {
        time::test::setSystemTimerProviderMock(&_timerMock);
        ON_CALL(_timerMock, getCurrentTimeInMs).WillByDefault([this] { return this->_timerCounter++; });
        logger_mock_.setup();
    }
    void TearDown() override
    {
        time::test::unsetSystemTimerProviderMock();
        logger_mock_.teardown();
    }

  protected:
    uint32_t _timerCounter{};
    testing::NiceMock<time::test::SystemTimerProviderMock> _timerMock{};
    middleware::logger::test::DslLogger logger_mock_{};
};

using TestConfigs =
    ::testing::Types<TestConfig<0U, 1U>, TestConfig<0U, 10U>, TestConfig<10U, 1U>, TestConfig<10U, 10U>>;
TYPED_TEST_SUITE(FutureDispatcherTestSuite, TestConfigs);

/**
 * @brief Test obtainRequestId for a single future object and check its requestId, futureDispatcherSlot and state.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestObtainRequestId)
{
    // ARRANGE
    HRESULT ret{};
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT
    ret = this->obtainRequestId(requestId, future);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::Ok);
    EXPECT_EQ(requestId, 0U);
    EXPECT_EQ(future.getRequestId(), 0U);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0U);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test obtainRequestId for two different future objects and check that requestId is incremented.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestObtainRequestIdForTwoFutures)
{
    // ARRANGE
    uint16_t requestId1{};
    uint16_t requestId2{};
    typename TypeParam::FutureUnderTest future1{};
    typename TypeParam::FutureUnderTest future2{};

    // ACT
    this->obtainRequestId(requestId1, future1);
    this->invalidateFuture(future1);
    this->obtainRequestId(requestId2, future2);
    this->invalidateFuture(future2);

    // ASSERT
    EXPECT_EQ(requestId1, 0U);
    EXPECT_EQ(requestId2, 1U);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test obtainRequestId when it is called with the same future object and check that it is reused correctly with
 * a different requestId.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestObtainRequestIdForSameFuture)
{
    // ARRANGE
    HRESULT ret{};
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT
    static_cast<void>(this->obtainRequestId(requestId, future));
    ret = this->obtainRequestId(requestId, future);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::Ok);
    EXPECT_EQ(requestId, 1U);
    EXPECT_EQ(future.getRequestId(), 1U);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0U);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test obtainRequestId with a future that has been used by other FutureDispatcher and check that it returns an
 * error.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestObtainRequestIdForFutureUsedInOtherDispatcher)
{
    // ARRANGE
    HRESULT ret{};
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};
    etl::remove_pointer_t<decltype(this)> otherDispatcher{};

    // ACT
    otherDispatcher.obtainRequestId(requestId, future);
    ret = this->obtainRequestId(requestId, future);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::FutureAlreadyInUse);
    EXPECT_EQ(requestId, INVALID_REQUEST_ID);
    EXPECT_EQ(future.getRequestId(), 0U);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0U);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test obtainRequestId when it is called after the dispatcher reaches its maximum number of futures, and check
 * that it returns an error.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestObtainRequestIdAboveDispatcherLimit)
{
    // ARRANGE
    static constexpr uint16_t REQUEST_LIMIT = TypeParam::REQUEST_LIMIT;
    using FutureUnderTest = typename TypeParam::FutureUnderTest;
    uint16_t requestId{};
    FutureUnderTest future{};
    etl::array<uint16_t, REQUEST_LIMIT> requestIdArray{};
    etl::array<FutureUnderTest, REQUEST_LIMIT> futureArray = etl::array<FutureUnderTest, REQUEST_LIMIT>();

    // ACT
    for (size_t idx = 0U; idx < REQUEST_LIMIT; ++idx)
    {
        this->obtainRequestId(requestIdArray.at(idx), futureArray.at(idx));
    }

    // ASSERT
    EXPECT_EQ(this->obtainRequestId(requestId, future),
              HRESULT::RequestPoolDepleted);   // New obtainRequestId should execute with error
    EXPECT_EQ(requestId, INVALID_REQUEST_ID);  // requestId should be invalid after calling obtainRequestId

    this->freeAll();  // We need to call this here because the futureArray will get destroyed before the dispatcher
}

/**
 * @brief Test releaseRequestId for a message that has been created after successfully obtaining a requestId from a
 * future object.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestReleaseRequestId)
{
    // ARRANGE
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT
    this->obtainRequestId(requestId, future);
    MiddlewareMessage::Header header{0U, 0U, future.getRequestId(), 0U};
    MiddlewareMessage msg = MiddlewareMessage::createResponse(header, ClusterId(), ClusterId(), 0U);

    // ACT & ASSERT
    EXPECT_EQ(this->releaseRequestId(msg), &future);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getState(), IFuture::State::Ready);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test releaseRequestId when there are two future objects in its internal vector.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestReleaseRequestIdTwoFutures)
{
    // ARRANGE
    uint16_t requestId1{};
    uint16_t requestId2{};
    typename TypeParam::FutureUnderTest future1{};
    typename TypeParam::FutureUnderTest future2{};

    // ACT
    this->obtainRequestId(requestId1, future1);
    this->obtainRequestId(requestId2, future2);
    MiddlewareMessage::Header header{0U, 0U, future1.getRequestId(), 0U};
    MiddlewareMessage msg = MiddlewareMessage::createResponse(header, ClusterId(), ClusterId(), 0U);

    // ACT & ASSERT
    EXPECT_EQ(this->releaseRequestId(msg), &future1);
    EXPECT_EQ(future1.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future1.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
    EXPECT_EQ(future1.getState(), IFuture::State::Ready);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test releaseRequestId for a message that contains a requestId which is not the same as the requestId of the
 * future that is stored inside the dispatcher.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestReleaseRequestIdForInvalidFuture)
{
    // ARRANGE
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future1{};
    typename TypeParam::FutureUnderTest future2{};

    // ACT
    this->obtainRequestId(requestId, future1);
    MiddlewareMessage::Header header{0U, 0U, future2.getRequestId(), 0U};
    MiddlewareMessage msg = MiddlewareMessage::createResponse(header, ClusterId(), ClusterId(), 0U);

    this->logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                        logger::Error::SendMessage,
                                        HRESULT::FutureNotFound,
                                        msg.getSourceClusterId(),
                                        msg.getTargetClusterId(),
                                        msg.getHeader().serviceId,
                                        msg.getHeader().serviceInstanceId,
                                        msg.getHeader().memberId,
                                        msg.getHeader().requestId);

    IFuture* ret = this->releaseRequestId(msg);

    // ASSERT
    EXPECT_EQ(ret, nullptr);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test invalidateFuture with a future that was stored inside the dispatcher and check if its state has been
 * reset.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestInvalidateFutureStoredInsideDispatcher)
{
    // ARRANGE
    HRESULT ret{};
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT
    this->obtainRequestId(requestId, future);
    ret = this->invalidateFuture(future);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::Ok);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test invalidateFuture with a future that was stored inside other dispatcher and check the error return code
 * and that the future's state remains the same.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestInvalidateFutureWithInvalidFuture)
{
    // ARRANGE
    HRESULT ret{};
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};
    etl::remove_pointer_t<decltype(this)> otherDispatcher{};

    // ACT
    otherDispatcher.obtainRequestId(requestId, future);
    ret = this->invalidateFuture(future);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::InstanceNotFound);
    EXPECT_EQ(future.getRequestId(), 0U);                   // Future state should remain the same
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0U);        // Future state should remain the same
    EXPECT_EQ(future.getState(), IFuture::State::Pending);  // Future state should remain the same

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test freeAll and check that it resets the state of futures that are stored inside the dispatcher.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestFreeAll)
{
    // ARRANGE
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT
    this->obtainRequestId(requestId, future);
    this->freeAll();

    // ASSERT
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test freeAll when dispather is storing several futures inside and check that all of their states have been
 * reset and that the requestId counter of this dispatcher has been reset.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestFreeAllWhenSeveralFuturesAreStored)
{
    // ARRANGE
    static constexpr uint16_t REQUEST_LIMIT = TypeParam::REQUEST_LIMIT;
    using FutureUnderTest = typename TypeParam::FutureUnderTest;
    etl::array<uint16_t, REQUEST_LIMIT> requestIdArray{};
    etl::array<FutureUnderTest, REQUEST_LIMIT> futureArray = etl::array<FutureUnderTest, REQUEST_LIMIT>();

    // ACT
    for (size_t idx = 0U; idx < REQUEST_LIMIT; ++idx)
    {
        this->obtainRequestId(requestIdArray.at(idx), futureArray.at(idx));
    }
    this->freeAll();

    // ASSERT
    for (size_t idx = 0U; idx < REQUEST_LIMIT; ++idx)
    {
        EXPECT_EQ(futureArray.at(idx).getRequestId(), INVALID_REQUEST_ID);
        EXPECT_EQ(futureArray.at(idx).getFutureDispatcherSlot(), INVALID_REQUEST_ID);
        EXPECT_EQ(futureArray.at(idx).getState(), IFuture::State::Invalid);
    }
    EXPECT_EQ(this->obtainRequestId(requestIdArray.at(0U), futureArray.at(0U)),
              HRESULT::Ok);                // New obtainRequestId should execute without error
    EXPECT_EQ(requestIdArray.at(0U), 0U);  // first requestId should be 0, after calling freeAll

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test releaseRequestId for a message that contains an error.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestReleaseRequestIdForMessageWithError)
{
    // ARRANGE
    auto expectedValues = etl::make_array<etl::pair<ErrorState, IFuture::State>>(
        etl::make_pair(ErrorState::UserDefinedError, IFuture::State::UserError),
        etl::make_pair(ErrorState::ServiceBusy, IFuture::State::ServiceBusy),
        etl::make_pair(ErrorState::ServiceNotFound, IFuture::State::ServiceNotFound),
        etl::make_pair(ErrorState::SerializationError, IFuture::State::SerializationError),
        etl::make_pair(ErrorState::DeserializationError, IFuture::State::DeserializationError),
        etl::make_pair(ErrorState::QueueFullError, IFuture::State::CouldNotDeliverError));
    uint16_t requestId{};
    typename TypeParam::FutureUnderTest future{};

    // ACT & ASSERT
    for (auto pair : expectedValues)
    {
        this->obtainRequestId(requestId, future);
        MiddlewareMessage::Header header{0U, 0U, 0U, future.getRequestId()};
        auto msg = MiddlewareMessage::createErrorResponse(header, ClusterId(), ClusterId(), 0U, pair.first);
        EXPECT_EQ(this->releaseRequestId(msg), &future);
        EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
        EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
        EXPECT_EQ(future.getState(), pair.second);
        this->freeAll();
    }

    this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
}

/**
 * @brief Test dispatcher when it is storing future objects with timeouts, and when a timeout occurs in a future.
 *
 */
TYPED_TEST(FutureDispatcherTestSuite, TestTimeoutDispatching)
{
    using FutureUnderTest = typename TypeParam::FutureUnderTest;
    if constexpr (FutureUnderTest::Traits::TIMEOUT_VALUE > 0U)
    {
        // ARRANGE
        using RefApp = typename TestFixture::RefApp;
        uint16_t requestId{};
        FutureUnderTest futureWithTimeout{};
        testing::NiceMock<RefApp> _appMock{};
        futureWithTimeout.setReceiveHandler(
            FutureUnderTest::Callback::template create<RefApp, &RefApp::receiveFutureData>(_appMock));

        // ACT
        this->obtainRequestId(requestId, futureWithTimeout);
        for (size_t timeCounter = 0U; timeCounter < (FutureUnderTest::Traits::TIMEOUT_VALUE); ++timeCounter)
        {
            this->updateTimeouts();
        }

        // ASSERT
        EXPECT_CALL(_appMock,
                    receiveFutureData(
                        testing::Truly([](const etl::expected<typename FutureUnderTest::Type, IFuture::State>& exp) {
                            return exp.error() == IFuture::State::Timeout;
                        })))
            .Times(1U);
        this->updateTimeouts();
        EXPECT_EQ(futureWithTimeout.getRequestId(), INVALID_REQUEST_ID);
        EXPECT_EQ(futureWithTimeout.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
        EXPECT_EQ(futureWithTimeout.getState(), IFuture::State::Timeout);

        this->freeAll();  // We need to call this here to cleanup dispatcher before future objects are destroyed.
    }
    else
    {
        GTEST_SKIP() << "Test should only run for dispatchers that contain future objects with timeouts bigger than 0.";
    }
}

}  // namespace middleware::core::test
