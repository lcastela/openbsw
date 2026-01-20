#pragma once

#include <etl/delegate.h>
#include <etl/type_traits.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"

namespace middleware::core
{
template <typename EventType>
struct EventCallbackTypeSelector
{
    using Callback = ::etl::delegate<void(const EventType&)>;
};

template <>
struct EventCallbackTypeSelector<void>
{
    using Callback = ::etl::delegate<void()>;
};

template <class Proxy, typename EventType>
class ProxyEventBase
{
    friend Proxy;

  public:
    using OnFieldChangedCallback = typename EventCallbackTypeSelector<EventType>::Callback;
    using Callback = OnFieldChangedCallback;

    ProxyEventBase() = default;
    ~ProxyEventBase() noexcept { unsetReceiveHandler(); }
    ProxyEventBase& operator=(const ProxyEventBase&) = delete;
    ProxyEventBase(const ProxyEventBase&) = delete;
    ProxyEventBase& operator=(ProxyEventBase&&) = delete;
    ProxyEventBase(ProxyEventBase&&) = delete;

    void setReceiveHandler(const OnFieldChangedCallback callback) noexcept { cbk_ = callback; }

    void unsetReceiveHandler() noexcept { cbk_ = OnFieldChangedCallback(); }

  private:
    void setEvent_([[maybe_unused]] const MiddlewareMessage& msg) const
    {
        if constexpr (etl::is_void_v<EventType>)
        {
            cbk_.call_if();
        }
        else
        {
            const EventType& data = MessageAllocator::getInstance().readPayload<EventType>(msg);
            cbk_.call_if(data);
        }
    }

    OnFieldChangedCallback cbk_;
};

}  // namespace middleware::core
