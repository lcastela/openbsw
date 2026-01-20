#include <stdint.h>

#include <etl/delegate.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/proxy_attribute.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/types.h"
#include "middleware_instances_database.h"

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
constexpr MemberId set_id = 1;
}  // namespace internal

class NoSubscriptionsProxy;

using ArgType = bool;
using GetterFuturePolicy = Future<FutureTraits<ArgType, internal::get_id, false>>;
using SetterFuturePolicy = void;

using NoSubscriptionsAttribute = ProxyAttribute<NoSubscriptionsProxy,
                                                GetterFuturePolicy,
                                                1,
                                                SetterFuturePolicy,
                                                AttributeType::NoSubscriptions,
                                                ArgType>;

struct DerivedNoSubscriptionsAttribute final : public NoSubscriptionsAttribute
{
    using AttributeType = ArgType;
    using Base = NoSubscriptionsAttribute;
    using GetterFuture = GetterFuturePolicy;
    DerivedNoSubscriptionsAttribute() = default;

    HRESULT set(const ArgType& value) { return Base::set(value, internal::set_id); }

    HRESULT invalidateFuture(GetterFuture& future) { return Base::invalidateFuture(future); }
};

class NoSubscriptionsProxy : public ::middleware::core::ProxyBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));

    NoSubscriptionsProxy() : ProxyBase(), attribute()
    {
        init();
        attribute.init(this);
        attribute.freeAll();
    }
    ~NoSubscriptionsProxy() override { attribute.freeAll(); }

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
                    (static_cast<typename DerivedNoSubscriptionsAttribute::GetterFuture*>(f))->setResult(msg);
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

    DerivedNoSubscriptionsAttribute attribute;
};

class ProxyNoSubscriptionsAttributeTest : public ::testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ProxyNoSubscriptionsAttributeTest, NoSubscriptionsAttributeGetEventOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<NoSubscriptionsProxy> proxy;
    proxy.deInit();
    DerivedNoSubscriptionsAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.get(future),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyNoSubscriptionsAttributeTest, NoSubscriptionsAttributeGetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<NoSubscriptionsProxy> proxy;
    DerivedNoSubscriptionsAttribute::GetterFuture future;

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

TEST_F(ProxyNoSubscriptionsAttributeTest, NoSubscriptionsAttributeGetAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<NoSubscriptionsProxy> proxy;
    DerivedNoSubscriptionsAttribute::GetterFuture future;

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

TEST_F(ProxyNoSubscriptionsAttributeTest, NoSubscriptionsAttributeSetOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<NoSubscriptionsProxy> proxy;
    proxy.deInit();
    const ArgType payload = true;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.set(payload),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
}

TEST_F(ProxyNoSubscriptionsAttributeTest, NoSubscriptionsAttributeSet)
{
    // ARRANGE
    NiceMock<NoSubscriptionsProxy> proxy;
    const ArgType payload = true;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.set(payload), HRESULT::Ok);
}

}  // namespace test
}  // namespace core
}  // namespace middleware
