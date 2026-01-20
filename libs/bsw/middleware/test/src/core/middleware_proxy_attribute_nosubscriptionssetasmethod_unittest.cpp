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
constexpr MemberId set_id = 1;
}  // namespace internal

class NoSubscriptionsSetAsMethodProxy;

using ArgType = bool;
using GetterFuturePolicy = void;
using SetterFuturePolicy = Future<FutureTraits<ArgType, internal::set_id, false, 5000>>;

using NoSubscriptionsSetAsMethodAttribute = ProxyAttribute<NoSubscriptionsSetAsMethodProxy,
                                                           GetterFuturePolicy,
                                                           1,
                                                           SetterFuturePolicy,
                                                           AttributeType::NoSubscriptions_SetAsMethod,
                                                           ArgType>;

struct DerivedNoSubscriptionsSetAsMethodAttribute final : public NoSubscriptionsSetAsMethodAttribute
{
    using AttributeType = ArgType;
    using Base = NoSubscriptionsSetAsMethodAttribute;
    using SetterFuture = SetterFuturePolicy;
    DerivedNoSubscriptionsSetAsMethodAttribute() = default;

    HRESULT set(const ArgType& value, SetterFuture& future) { return Base::set(value, future, internal::set_id); }

    HRESULT invalidateFuture(SetterFuture& future) { return Base::invalidateFuture(future, internal::set_id); }
};

class NoSubscriptionsSetAsMethodProxy : public ::middleware::core::ProxyBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));

    NoSubscriptionsSetAsMethodProxy() : ProxyBase(), attribute()
    {
        init();
        attribute.init(this);
        attribute.freeAll();
    }
    ~NoSubscriptionsSetAsMethodProxy() override { attribute.freeAll(); }

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
            case internal::set_id: {
                IFuture* const f = attribute.releaseRequestId(msg);
                if (nullptr != f)
                {
                    (static_cast<typename DerivedNoSubscriptionsSetAsMethodAttribute::SetterFuture*>(f))
                        ->setResult(msg);
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

    DerivedNoSubscriptionsSetAsMethodAttribute attribute;
};

class ProxyNoSubscriptionsSetAsMethodAttributeTest : public ::testing::Test
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

TEST_F(ProxyNoSubscriptionsSetAsMethodAttributeTest,
       NoSubscriptionsSetAsMethodAttributeSetEventAndRespectiveResponseOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<NoSubscriptionsSetAsMethodProxy> proxy;
    proxy.deInit();
    DerivedNoSubscriptionsSetAsMethodAttribute::SetterFuture future;
    const bool appStarted = true;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.set(appStarted, future),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyNoSubscriptionsSetAsMethodAttributeTest, NoSubscriptionsSetAsMethodAttributeSetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<NoSubscriptionsSetAsMethodProxy> proxy;
    DerivedNoSubscriptionsSetAsMethodAttribute::SetterFuture future;
    const bool appStarted = true;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.set(appStarted, future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    proxy.attribute.updateTimeouts();

    MiddlewareMessage::Header header{0, internal::set_id, 0, 0};
    MiddlewareMessage msg =
        MiddlewareMessage::createRequest(header, static_cast<ClusterId>(0), static_cast<ClusterId>(1), 0);
    EXPECT_EQ(MessageAllocator::getInstance().allocate<bool>(appStarted, msg), HRESULT::Ok);
    EXPECT_EQ(future.getRequestId(), msg.getHeader().requestId);

    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Ready);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyNoSubscriptionsSetAsMethodAttributeTest, NoSubscriptionsSetAsMethodAttributeSetEventAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<NoSubscriptionsSetAsMethodProxy> proxy;
    DerivedNoSubscriptionsSetAsMethodAttribute::SetterFuture future;
    const bool appStarted = true;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.set(appStarted, future), HRESULT::Ok);  // no cluster connections nor database
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
