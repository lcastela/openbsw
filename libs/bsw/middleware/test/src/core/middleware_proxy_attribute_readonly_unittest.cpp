#include <stdint.h>

#include <etl/delegate.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
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
constexpr MemberId event_id = 2U;
}  // namespace internal

class ReadOnlyProxy;

using ArgType = uint8_t;
using GetterFuturePolicy = Future<FutureTraits<ArgType, internal::get_id, false>>;
using SetterFuturePolicy = void;

using ReadOnlyAttribute =
    ProxyAttribute<ReadOnlyProxy, GetterFuturePolicy, 1, SetterFuturePolicy, AttributeType::ReadOnly, ArgType>;

class DerivedReadOnlyAttribute final : public ReadOnlyAttribute
{
  public:
    using AttributeType = ArgType;
    using Base = ReadOnlyAttribute;
    using OnFieldChangedCallback = Base::OnFieldChangedCallback;
    using GetterFuture = GetterFuturePolicy;
    DerivedReadOnlyAttribute() : Base() {}

    HRESULT invalidateFuture(GetterFuture& future) { return Base::invalidateFuture(future); }
};

class ReadOnlyProxy : public ::middleware::core::ProxyBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));

    ReadOnlyProxy() : ProxyBase(), attribute()
    {
        init();
        attribute.init(this);
        attribute.freeAll();
    }
    ~ReadOnlyProxy() override { attribute.freeAll(); }

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
                    (static_cast<typename DerivedReadOnlyAttribute::GetterFuture*>(f))->setResult(msg);
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

    DerivedReadOnlyAttribute attribute;
};

class ProxyReadOnlyAttributeTest : public ::testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ProxyReadOnlyAttributeTest, ReadOnlyAttributeGetEventAndRespectiveResponseOnAnUnregisteredProxy)
{
    // ARRANGE
    NiceMock<ReadOnlyProxy> proxy;
    proxy.deInit();
    DerivedReadOnlyAttribute::GetterFuture future;

    // ACT & ASSERT
    EXPECT_EQ(proxy.attribute.get(future),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
    EXPECT_EQ(future.getState(), IFuture::State::Invalid);
    EXPECT_EQ(future.getRequestId(), INVALID_REQUEST_ID);
    EXPECT_EQ(future.getFutureDispatcherSlot(), INVALID_REQUEST_ID);
}

TEST_F(ProxyReadOnlyAttributeTest, ReadOnlyAttributeGetEventAndRespectiveResponse)
{
    // ARRANGE
    NiceMock<ReadOnlyProxy> proxy;
    DerivedReadOnlyAttribute::GetterFuture future;

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

TEST_F(ProxyReadOnlyAttributeTest, ReadOnlyAttributeGetAndInvalidateFuture)
{
    // ARRANGE
    NiceMock<ReadOnlyProxy> proxy;
    DerivedReadOnlyAttribute::GetterFuture future;

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

TEST_F(ProxyReadOnlyAttributeTest, ReadOnlyAttributeSetEvent)
{
    // ARRANGE
    NiceMock<ReadOnlyProxy> proxy;
    MiddlewareMessage msg = MiddlewareMessage::createEvent(0, internal::event_id, 0, static_cast<ClusterId>(0));
    msg.setTargetClusterId(static_cast<ClusterId>(1));
    const ArgType payload = 1U;

    auto cb = [payload](const ArgType& event) { EXPECT_EQ(event, payload); };
    proxy.attribute.setReceiveHandler(DerivedReadOnlyAttribute::OnFieldChangedCallback::create(cb));

    // ACT & ASSERT
    EXPECT_EQ(MessageAllocator::getInstance().allocate<ArgType>(payload, msg), HRESULT::Ok);
    EXPECT_EQ(proxy.onNewMessageReceived(msg), HRESULT::Ok);
}

}  // namespace test
}  // namespace core
}  // namespace middleware
