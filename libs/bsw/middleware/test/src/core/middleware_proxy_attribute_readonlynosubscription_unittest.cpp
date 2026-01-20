#include <stdint.h>

#include <etl/delegate.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/proxy_attribute.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/types.h"
#include "middleware_instances_database.h"
#include "mock/system_timer_provider_mock.h"

using testing::_;
using testing::Exactly;
using testing::NiceMock;

namespace middleware
{
namespace core
{
namespace test
{

namespace internal
{
constexpr MemberId get_id = 0;
}  // namespace internal

class ReadOnlyNoSubscriptionProxy;

using ArgType = uint32_t;
using GetterFuturePolicy = Future<FutureTraits<ArgType, internal::get_id, false, 5000>>;
using SetterFuturePolicy = void;

using ReadOnlyNoSubscriptionAttribute = ProxyAttribute<ReadOnlyNoSubscriptionProxy,
                                                       GetterFuturePolicy,
                                                       1,
                                                       SetterFuturePolicy,
                                                       AttributeType::ReadOnly_NoSubscription,
                                                       ArgType>;

struct DerivedReadOnlyNoSubscriptionAttribute final : public ReadOnlyNoSubscriptionAttribute
{
    using AttributeType = ArgType;
    using Base = ReadOnlyNoSubscriptionAttribute;
    using GetterFuture = GetterFuturePolicy;
    DerivedReadOnlyNoSubscriptionAttribute() = default;

    HRESULT invalidateFuture(GetterFuture& future) { return Base::invalidateFuture(future); }
};

class ReadOnlyNoSubscriptionProxy : public ::middleware::core::ProxyBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));

    ReadOnlyNoSubscriptionProxy() : ProxyBase(), attribute()
    {
        init();
        attribute.init(this);
        attribute.freeAll();
    }
    ~ReadOnlyNoSubscriptionProxy() override { attribute.freeAll(); }

    HRESULT init()
    {
        return ProxyBase::initFromInstancesDatabase(1U, static_cast<core::ClusterId>(1U), etl::span(INSTANCESDATABASE));
    }

    void deInit() { ProxyBase::unsubscribe(0); }

    HRESULT onNewMessageReceived(MiddlewareMessage const& msg) override
    {
        HRESULT ret = HRESULT::Ok;
        switch (msg.getHeader().memberId)
        {
            case internal::get_id: /* get event */
            {
                IFuture* const f = attribute.releaseRequestId(msg);
                if (nullptr != f)
                {
                    (static_cast<typename DerivedReadOnlyNoSubscriptionAttribute::GetterFuture*>(f))->setResult(msg);
                }
                break;
            }
            default: {
                ret = HRESULT::ServiceMemberIdNotFound;
                break;
            }
        }
        return ret;
    }

    DerivedReadOnlyNoSubscriptionAttribute attribute;
};

class ProxyReadOnlyNoSubscriptionAttributeTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        time::test::setSystemTimerProviderMock(&_timerMock);
        ON_CALL(_timerMock, getCurrentTimeInMs).WillByDefault([this] { return this->_timerCounter++; });
    }

    void TearDown() override { time::test::unsetSystemTimerProviderMock(); }

  protected:
    uint32_t _timerCounter{};
    NiceMock<time::test::SystemTimerProviderMock> _timerMock{};
};

TEST_F(ProxyReadOnlyNoSubscriptionAttributeTest,
       ReadOnlyNoSubscriptionAttributeGetEventAndRespectiveResponseOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<ReadOnlyNoSubscriptionProxy> proxy;
    proxy.deInit();
    DerivedReadOnlyNoSubscriptionAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.get(future),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyReadOnlyNoSubscriptionAttributeTest, ReadOnlyNoSubscriptionAttributeGetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<ReadOnlyNoSubscriptionProxy> proxy;
    DerivedReadOnlyNoSubscriptionAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.get(future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    proxy.attribute.updateTimeouts();

    MiddlewareMessage::Header header{0, internal::get_id, 0, 0};
    MiddlewareMessage msg =
        MiddlewareMessage::createRequest(header, static_cast<ClusterId>(0), static_cast<ClusterId>(1), 0);
    EXPECT_EQ(future.getRequestId(), msg.getHeader().requestId);

    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Ready);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyReadOnlyNoSubscriptionAttributeTest, ReadOnlyNoSubscriptionAttributeGetAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<ReadOnlyNoSubscriptionProxy> proxy;
    DerivedReadOnlyNoSubscriptionAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.get(future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    proxy.attribute.updateTimeouts();

    EXPECT_EQ(proxy.attribute.invalidateFuture(future), HRESULT::Ok);

    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

}  // namespace test
}  // namespace core
}  // namespace middleware
