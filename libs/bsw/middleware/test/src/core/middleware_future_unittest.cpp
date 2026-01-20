#include <cstdint>

#include <etl/array.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "allocator_mock.h"
#include "middleware/core/future.h"
#include "middleware/core/ifuture.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{
namespace test
{

struct MyType
{
    uint16_t a{};
    uint16_t b{};
    uint16_t c{};
    uint16_t d{};

    bool operator==(const MyType& other) const
    {
        return (this->a == other.a) && (this->b == other.b) && (this->c == other.c) && (this->d == other.d);
    }
};

static constexpr uint32_t TIMEOUT_TEST_VALUE = 100U;

template <typename T>
class FutureTest : public ::testing::Test
{
  public:
    using FutureTraitsConfig = T;
    using FutureType = Future<FutureTraitsConfig>;

    class CallbackMock
    {
      public:
        MOCK_METHOD(void,
                    callback,
                    ((etl::expected<typename FutureTraitsConfig::ArgumentType, IFuture::State> &&)),
                    ());
    };

    void SetUp() final
    {
        memory::test::AllocatorMock::setAllocatorMock(allocatorMock_);

        callback_ = FutureType::Callback::template create<CallbackMock, &CallbackMock::callback>(callbackMock_);
    }

    MiddlewareMessage CREATE_MIDDLEWARE_MSG() const
    {
        const MiddlewareMessage::Header header{0U, 0U, 0U, 0U};
        MiddlewareMessage msg =
            MiddlewareMessage::createResponse(header, static_cast<ClusterId>(0U), static_cast<ClusterId>(0U), 0U);

        if constexpr (etl::is_void<typename FutureTraitsConfig::ArgumentType>::value == false)
        {
            const HRESULT ret =
                MessageAllocator::getInstance().allocate(typename FutureTraitsConfig::ArgumentType(), msg);
            EXPECT_EQ(ret, HRESULT::Ok);
        }
        return msg;
    }

    template <typename Type = typename T::ArgumentType>
    typename etl::enable_if<!etl::is_void<Type>::value, void>::type EXPECT_CALLBACK(
        Type&& value,
        const ::testing::Cardinality& cardinality = ::testing::Exactly(1))
    {
        EXPECT_CALL(this->callbackMock_,
                    callback(testing::Truly([&value](const etl::expected<Type, IFuture::State>& exp) {
                        return exp.has_value() && exp.value() == value;
                    })))
            .Times(cardinality);
    }

    template <typename Type = typename T::ArgumentType>
    typename etl::enable_if<etl::is_void<Type>::value, void>::type EXPECT_CALLBACK(
        const ::testing::Cardinality& cardinality = ::testing::Exactly(1))
    {
        EXPECT_CALL(
            this->callbackMock_,
            callback(testing::Truly([](const etl::expected<void, IFuture::State>& exp) { return exp.has_value(); })))
            .Times(cardinality);
    }

    void EXPECT_ERROR_CALLBACK(IFuture::State&& state,
                               const ::testing::Cardinality& cardinality = ::testing::Exactly(1))
    {
        EXPECT_CALL(this->callbackMock_,
                    callback(testing::Truly(
                        [&state](const etl::expected<typename FutureTraitsConfig::ArgumentType, IFuture::State>& exp) {
                            return exp.error() == state;
                        })))
            .Times(cardinality);
    }

    CallbackMock callbackMock_;
    typename FutureType::Callback callback_;

  private:
    testing::NiceMock<memory::test::AllocatorMock> allocatorMock_;
};

using TestTypes = ::testing::Types<FutureTraits<MyType, 0U, false, 0U>,
                                   FutureTraits<void, 0U, false, 0U>,
                                   FutureTraits<MyType, 0U, false, TIMEOUT_TEST_VALUE>,
                                   FutureTraits<void, 0U, false, TIMEOUT_TEST_VALUE>>;

TYPED_TEST_SUITE(FutureTest, TestTypes);

/**
 * @brief Test getters and setters
 *
 */
TYPED_TEST(FutureTest, TestGettersAndSetters)
{
    // ARRANGE
    typename TestFixture::FutureType future{};
    IFuture::State state{IFuture::State::Timeout};
    RequestId requestId{0xF192U};
    RequestId futureDispatcherSlot{0x0010U};

    // ACT
    future.setState(state);
    future.setRequestId(requestId);
    future.setFutureDispatcherSlot(futureDispatcherSlot);

    // ASSERT
    EXPECT_EQ(future.getState(), state);
    EXPECT_EQ(future.getRequestId(), requestId);
    EXPECT_EQ(future.getFutureDispatcherSlot(), futureDispatcherSlot);
}

TYPED_TEST(FutureTest, TestTimeoutGettersAndSetters)
{
    if constexpr (TestFixture::FutureType::Traits::TIMEOUT_VALUE > 0U)
    {
        // ARRANGE
        typename TestFixture::FutureType future{};

        // ACT & ASSERT
        future.setCallerTimestamp(TIMEOUT_TEST_VALUE / 2);
        EXPECT_EQ(future.getCallerTimestamp(), TIMEOUT_TEST_VALUE / 2);
    }
    else
    {
        GTEST_SKIP() << "Test should only run for futures that contain timeouts bigger than 0.";
    }
}

TYPED_TEST(FutureTest, TestFutureDefaultState)
{
    // ARRANGE
    typename TestFixture::FutureType future{};

    // ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_FALSE(future.isReady());
    EXPECT_FALSE(future.hasError());
}

TYPED_TEST(FutureTest, TestFuturePendingState)
{
    // ARRANGE
    typename TestFixture::FutureType future{};

    // ACT & ASSERT
    future.setState(IFuture::State::Pending);
    EXPECT_FALSE(future.isReady());
    EXPECT_FALSE(future.hasError());
}

TYPED_TEST(FutureTest, TestFutureReadyState)
{
    // ARRANGE
    typename TestFixture::FutureType future{};

    // ACT & ASSERT
    future.setState(IFuture::State::Ready);
    EXPECT_TRUE(future.isReady());
    EXPECT_FALSE(future.hasError());
}

TYPED_TEST(FutureTest, TestFutureErrorStates)
{
    // ARRANGE
    typename TestFixture::FutureType future{};
    etl::array<IFuture::State, 7U> states{IFuture::State::Timeout,
                                          IFuture::State::UserError,
                                          IFuture::State::ServiceBusy,
                                          IFuture::State::ServiceNotFound,
                                          IFuture::State::SerializationError,
                                          IFuture::State::DeserializationError,
                                          IFuture::State::CouldNotDeliverError};

    // ACT & ASSERT
    for (auto state : states)
    {
        future.setState(state);

        EXPECT_FALSE(future.isReady());
        EXPECT_TRUE(future.hasError());
    }
}

/**
 * @brief Test setResult method when state is State::Ready, which will call the registered callback with a valid pointer
 * to the payload.
 *
 */
TYPED_TEST(FutureTest, TestSetResultWithStateReady)
{
    // ARRANGE
    typename TestFixture::FutureType future{};

    // ACT & ASSERT
    future.setState(IFuture::State::Ready);
    future.setReceiveHandler(this->callback_);
    if constexpr (!etl::is_void<typename TestFixture::FutureTraitsConfig::ArgumentType>::value)
    {
        this->EXPECT_CALLBACK(typename TestFixture::FutureTraitsConfig::ArgumentType());
    }
    else
    {
        this->EXPECT_CALLBACK();
    }
    future.setResult(this->CREATE_MIDDLEWARE_MSG());
}

/**
 * @brief Test setResult method when state is State::Invalid, which will call the registered callback with an
 * invalid pointer (nullptr).
 *
 */
TYPED_TEST(FutureTest, TestSetResultWithStateInvalid)
{
    // ARRANGE
    typename TestFixture::FutureType future{};
    etl::array<IFuture::State, 8U> states{IFuture::State::Pending,
                                          IFuture::State::Timeout,
                                          IFuture::State::UserError,
                                          IFuture::State::ServiceBusy,
                                          IFuture::State::ServiceNotFound,
                                          IFuture::State::SerializationError,
                                          IFuture::State::DeserializationError,
                                          IFuture::State::CouldNotDeliverError};

    future.setReceiveHandler(this->callback_);

    // ACT & ASSERT
    for (auto state : states)
    {
        future.setState(state);
        this->EXPECT_ERROR_CALLBACK(etl::move(state));
        future.setResult(this->CREATE_MIDDLEWARE_MSG());
    }
}

/**
 * @brief Test unsetReceiveHandler method, which will cause the callback attribute of Future to be reset, so that
 if
 * setResult is called, no callback will invoked.
 *
 */
TYPED_TEST(FutureTest, TestUnsetReceiveHandler)
{
    // ARRANGE
    typename TestFixture::FutureType future{};
    future.setReceiveHandler(this->callback_);

    // ACT & ASSERT
    future.unsetReceiveHandler();
    EXPECT_CALL(this->callbackMock_, callback(::testing::_)).Times(0);
    future.setResult(this->CREATE_MIDDLEWARE_MSG());
}

TYPED_TEST(FutureTest, TestFutureWithTimeout)
{
    if constexpr (TestFixture::FutureType::Traits::TIMEOUT_VALUE > 0U)
    {
        // ARRANGE
        typename TestFixture::FutureType future{};
        future.setReceiveHandler(this->callback_);

        // ACT & ASSERT
        this->EXPECT_ERROR_CALLBACK(IFuture::State::Timeout);
        future.timeoutExpired();
    }
    else
    {
        GTEST_SKIP() << "Test should only run for futures that contain timeouts bigger than 0.";
    }
}

}  // namespace test
}  // namespace core
}  // namespace middleware
