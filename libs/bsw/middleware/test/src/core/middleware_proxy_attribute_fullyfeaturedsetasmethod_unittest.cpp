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
constexpr MemberId set_id = 1U;
constexpr MemberId event_id = 2U;
}  // namespace internal

class FullyFeaturedSetAsMethodProxy;

using ArgType = uint32_t;
using GetterFuturePolicy = Future<FutureTraits<ArgType, internal::get_id, false, 5000U>>;
using SetterFuturePolicy = Future<FutureTraits<ArgType, internal::set_id, false, 5000U>>;

using FullyFeaturedSetAsMethodAttribute = ProxyAttribute<FullyFeaturedSetAsMethodProxy,
                                                         GetterFuturePolicy,
                                                         1,
                                                         SetterFuturePolicy,
                                                         AttributeType::FullyFeatured_SetAsMethod,
                                                         ArgType>;

struct DerivedFullyFeaturedSetAsMethodAttribute final : public FullyFeaturedSetAsMethodAttribute
{
    using AttributeType = ArgType;
    using Base = FullyFeaturedSetAsMethodAttribute;
    using OnFieldChangedCallback = Base::OnFieldChangedCallback;
    using GetterFuture = GetterFuturePolicy;
    using SetterFuture = SetterFuturePolicy;
    DerivedFullyFeaturedSetAsMethodAttribute() = default;

    HRESULT set(const AttributeType& value, SetterFuture& future) { return Base::set(value, future, internal::set_id); }

    HRESULT invalidateFuture(SetterFuture& future) { return Base::invalidateFuture(future, internal::set_id); }

    HRESULT invalidateFuture(GetterFuture& future) { return Base::invalidateFuture(future, internal::get_id); }
};

class FullyFeaturedSetAsMethodProxy : public ::middleware::core::ProxyBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));

    FullyFeaturedSetAsMethodProxy() : ProxyBase(), attribute()
    {
        init();
        attribute.init(this);
        attribute.freeAll();
    }
    ~FullyFeaturedSetAsMethodProxy() override { attribute.freeAll(); }

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
                    (static_cast<typename DerivedFullyFeaturedSetAsMethodAttribute::GetterFuture*>(f))->setResult(msg);
                }
                break;
            }
            case internal::set_id: {
                IFuture* const f = attribute.releaseRequestId(msg);
                if (nullptr != f)
                {
                    (static_cast<typename DerivedFullyFeaturedSetAsMethodAttribute::SetterFuture*>(f))->setResult(msg);
                }
                break;
            }
            case internal::event_id: /* set event  */
            {
                attribute.setEvent_(msg);
                break;
            }
            default: {
                ret = HRESULT::ServiceMemberIdNotFound;
                break;
            }
        }
        return ret;
    }

    DerivedFullyFeaturedSetAsMethodAttribute attribute;
};

class ProxyFullyFeaturedSetAsMethodTest : public ::testing::Test
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

TEST_F(ProxyFullyFeaturedSetAsMethodTest, ReadOnlyAttributeGetEventOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    proxy.deInit();
    DerivedFullyFeaturedSetAsMethodAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.get(future),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, ReadOnlyAttributeGetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    DerivedFullyFeaturedSetAsMethodAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.get(future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    MiddlewareMessage::Header header{0, internal::get_id, 0, 0};
    MiddlewareMessage msg =
        MiddlewareMessage::createRequest(header, static_cast<ClusterId>(0), static_cast<ClusterId>(1), 0);
    EXPECT_EQ(future.getRequestId(), msg.getHeader().requestId);

    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Ready);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, ReadOnlyAttributeGetAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    DerivedFullyFeaturedSetAsMethodAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.get(future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    EXPECT_EQ(proxy.attribute.invalidateFuture(future), HRESULT::Ok);

    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, NoSubscriptionsSetAsMethodAttributeSetEventOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    proxy.deInit();
    DerivedFullyFeaturedSetAsMethodAttribute::SetterFuture future;
    const ArgType payload = 0xFFFFFFFFU;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.set(payload, future), HRESULT::NotRegistered);  // no cluster connections nor database
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, NoSubscriptionsSetAsMethodAttributeSetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    DerivedFullyFeaturedSetAsMethodAttribute::SetterFuture future;
    const ArgType payload = 0xFFFFFFFFU;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.set(payload, future), HRESULT::Ok);  // no cluster connections nor database
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    proxy.attribute.updateTimeouts();

    MiddlewareMessage::Header header{0, internal::set_id, 0, 0};
    MiddlewareMessage msg =
        MiddlewareMessage::createRequest(header, static_cast<ClusterId>(0), static_cast<ClusterId>(1), 0);
    EXPECT_EQ(future.getRequestId(), msg.getHeader().requestId);

    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Ready);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, NoSubscriptionsSetAsMethodAttributeSetEventAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    DerivedFullyFeaturedSetAsMethodAttribute::SetterFuture future;
    const ArgType payload = 0xFFFFFFFFU;

    // ACT & ASSERT
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);

    EXPECT_EQ(proxy.attribute.set(payload, future), HRESULT::Ok);
    EXPECT_EQ(future.getState(), IFuture::State::Pending);
    EXPECT_EQ(future.getRequestId(), 0);
    EXPECT_EQ(future.getFutureDispatcherSlot(), 0);

    proxy.attribute.updateTimeouts();

    EXPECT_EQ(proxy.attribute.invalidateFuture(future), HRESULT::Ok);

    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyFullyFeaturedSetAsMethodTest, ReadOnlyAttributeMethodSetEvent)
{
    // ARRANGE
    NiceMock<FullyFeaturedSetAsMethodProxy> proxy;
    MiddlewareMessage msg = MiddlewareMessage::createEvent(0, internal::event_id, 0, static_cast<ClusterId>(0));
    msg.setTargetClusterId(static_cast<ClusterId>(1));
    const ArgType payload = 0xFFFFFFFFU;

    auto cb = [payload](const ArgType& event) { EXPECT_EQ(event, payload); };
    proxy.attribute.setReceiveHandler(DerivedFullyFeaturedSetAsMethodAttribute::OnFieldChangedCallback::create(cb));

    // ACT & ASSERT
    EXPECT_EQ(MessageAllocator::getInstance().allocate(payload, msg), HRESULT::Ok);
    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
}

}  // namespace test
}  // namespace core
}  // namespace middleware
