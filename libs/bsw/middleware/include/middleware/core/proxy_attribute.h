#pragma once

#include <etl/type_traits.h>

#include "middleware/core/future_dispatcher_type_selector.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/proxy_attribute_base.h"
#include "middleware/core/proxy_event_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

// NOLINTBEGIN(fuchsia-multiple-inheritance): IPBD-47314 (Deviation Approved)
template <typename... T>
struct InheritanceDelegate : T...
{
};
// NOLINTEND(fuchsia-multiple-inheritance): IPBD-47314 (Deviation Approved)

enum class AttributeType : uint16_t
{
    ReadOnly_NoSubscription,
    ReadOnly,
    NoSubscriptions,
    FullyFeatured,
    NoSubscriptions_SetAsMethod,
    FullyFeatured_SetAsMethod
};

template <typename Proxy,
          AttributeType Type,
          typename ValueType,
          typename GetterFuture = void,
          ::uint16_t REQUEST_LIMIT = 0U,
          typename Specialization = void>
struct ProxyAttributeBaseSelector;

template <typename Proxy, AttributeType Type, typename ValueType, typename GetterFuture, ::uint16_t REQUEST_LIMIT>
struct ProxyAttributeBaseSelector<Proxy,
                                  Type,
                                  ValueType,
                                  GetterFuture,
                                  REQUEST_LIMIT,
                                  typename etl::enable_if<(Type == AttributeType::ReadOnly_NoSubscription) ||
                                                          (Type == AttributeType::NoSubscriptions) ||
                                                          (Type == AttributeType::NoSubscriptions_SetAsMethod)>::type>
{
    using type = ProxyAttributeBase<REQUEST_LIMIT, GetterFuture>;
};

template <typename Proxy, AttributeType Type, typename ValueType, typename GetterFuture, ::uint16_t REQUEST_LIMIT>
struct ProxyAttributeBaseSelector<
    Proxy,
    Type,
    ValueType,
    GetterFuture,
    REQUEST_LIMIT,
    typename etl::enable_if<(Type == AttributeType::ReadOnly) || (Type == AttributeType::FullyFeatured) ||
                            (Type == AttributeType::FullyFeatured_SetAsMethod)>::type>
{
    using type = InheritanceDelegate<ProxyAttributeBase<REQUEST_LIMIT, GetterFuture>, ProxyEventBase<Proxy, ValueType>>;
};

template <typename Proxy,
          typename GetterFuture,
          ::uint16_t REQUEST_LIMIT,
          typename SetterFuture,
          AttributeType Type,
          typename ValueType,
          typename Specialization = void>
class ProxyAttribute;

template <typename Proxy,
          typename GetterFuture,
          ::uint16_t REQUEST_LIMIT,
          typename SetterFuture,
          AttributeType Type,
          typename ValueType>
class ProxyAttribute<
    Proxy,
    GetterFuture,
    REQUEST_LIMIT,
    SetterFuture,
    Type,
    ValueType,
    typename etl::enable_if<(Type == AttributeType::ReadOnly_NoSubscription) || (Type == AttributeType::ReadOnly) ||
                            (Type == AttributeType::NoSubscriptions) || (Type == AttributeType::FullyFeatured)>::type>
    : public ProxyAttributeBaseSelector<Proxy, Type, ValueType, GetterFuture, REQUEST_LIMIT>::type
{
    using Base = ProxyAttributeBase<REQUEST_LIMIT, GetterFuture>;

  protected:
    ProxyAttribute() = default;
    ~ProxyAttribute() = default;

    template <AttributeType T = Type>
    typename etl::enable_if<(T == AttributeType::NoSubscriptions) || (T == AttributeType::FullyFeatured), HRESULT>::type
    set(const ValueType& payload, const MemberId id)
    {
        HRESULT ret = HRESULT::NotRegistered;
        if ((Base::fProxy != nullptr) && Base::fProxy->isInitialized())
        {
            MiddlewareMessage msg = Base::fProxy->generateMessageHeader(id);
            ret = MessageAllocator::getInstance().allocate(payload, msg);
            if (ret == HRESULT::Ok)
            {
                ret = Base::fProxy->sendMessage(msg);
                if (ret != HRESULT::Ok)
                {
                    MessageAllocator::deallocate(msg);
                }
            }
        }

        return ret;
    }
};

template <typename Proxy,
          typename GetterFuture,
          ::uint16_t REQUEST_LIMIT,
          typename SetterFuture,
          AttributeType Type,
          typename ValueType>
class ProxyAttribute<Proxy,
                     GetterFuture,
                     REQUEST_LIMIT,
                     SetterFuture,
                     Type,
                     ValueType,
                     typename etl::enable_if<(Type == AttributeType::NoSubscriptions_SetAsMethod) ||
                                             (Type == AttributeType::FullyFeatured_SetAsMethod)>::type>
    : public ProxyAttributeBaseSelector<Proxy, Type, ValueType, GetterFuture, REQUEST_LIMIT>::type
{
  public:
    using Base = ProxyAttributeBase<REQUEST_LIMIT, GetterFuture>;
    using SetterArgumentType = typename SetterFuture::Traits::ArgumentType;

    template <typename T = GetterFuture>
    typename etl::enable_if<!etl::is_void<T>::value, IFuture*>::type releaseRequestId(const MiddlewareMessage& msg)
    {
        IFuture* ret = nullptr;
        if (GetterFuture::Traits::METHOD_MEMBER_ID == msg.getHeader().memberId)
        {
            ret = Base::releaseRequestId(msg);
        }
        else
        {
            ret = fSetDispatcher.releaseRequestId(msg);
        }
        return ret;
    }

    template <typename T = GetterFuture>
    typename etl::enable_if<etl::is_void<T>::value, IFuture*>::type releaseRequestId(const MiddlewareMessage& msg)
    {
        return fSetDispatcher.releaseRequestId(msg);
    }

    template <typename T = GetterFuture>
    typename etl::enable_if<!etl::is_void<T>::value, HRESULT>::type invalidateFuture(IFuture& future, const MemberId id)
    {
        HRESULT ret = HRESULT::UnknownMessageType;
        if (GetterFuture::Traits::METHOD_MEMBER_ID == id)
        {
            ret = Base::invalidateFuture(future);
        }
        else
        {
            ret = fSetDispatcher.invalidateFuture(future);
        }
        return ret;
    }

    template <typename T = GetterFuture>
    typename etl::enable_if<etl::is_void<T>::value, HRESULT>::type invalidateFuture(IFuture& future, const MemberId)
    {
        return fSetDispatcher.invalidateFuture(future);
    }

    // suppress misra 14.7.1 next_construct: Might, but not have to be called on production.
    template <typename T = GetterFuture>
    typename etl::enable_if<(!etl::is_void<T>::value) && (SetterFuture::Traits::TIMEOUT_VALUE != 0U), void>::type
    updateTimeouts()
    {
        fSetDispatcher.updateTimeouts();
        Base::updateTimeouts();
    }

    // suppress misra 14.7.1 next_construct: Might, but not have to be called on production.
    template <typename T = GetterFuture>
    typename etl::enable_if<(etl::is_void<T>::value) && (SetterFuture::Traits::TIMEOUT_VALUE != 0U), void>::type
    updateTimeouts()
    {
        fSetDispatcher.updateTimeouts();
    }

    // suppress misra 14.7.1 next_construct: Might, but not have to be called on production.
    template <typename T = GetterFuture>
    typename etl::enable_if<!etl::is_void<T>::value, void>::type freeAll()
    {
        fSetDispatcher.freeAll();
        Base::freeAll();
    }

    // suppress misra 14.7.1 next_construct: Might, but not have to be called on production.
    template <typename T = GetterFuture>
    typename etl::enable_if<etl::is_void<T>::value, void>::type freeAll()
    {
        fSetDispatcher.freeAll();
    }

  protected:
    ProxyAttribute() = default;
    ~ProxyAttribute() = default;

    HRESULT
    set(const SetterArgumentType& payload, SetterFuture& future, const MemberId id)
    {
        HRESULT res = HRESULT::NotRegistered;
        if ((Base::fProxy != nullptr) && Base::fProxy->isInitialized())
        {
            uint16_t requestId = middleware::core::INVALID_REQUEST_ID;
            res = fSetDispatcher.obtainRequestId(requestId, future);
            MiddlewareMessage msg = Base::fProxy->generateMessageHeader(id, requestId);
            if (res == HRESULT::Ok)
            {
                res = MessageAllocator::getInstance().allocate(payload, msg);
                if (res == HRESULT::Ok)
                {
                    res = Base::fProxy->sendMessage(msg);
                    if (res != HRESULT::Ok)
                    {
                        MessageAllocator::deallocate(msg);
                    }
                }
            }
            else
            {
                logger::logMessageSendingFailure(logger::LogLevel::Error, logger::Error::SendMessage, res, msg);
            }

            if (res != HRESULT::Ok)
            {
                static_cast<void>(fSetDispatcher.invalidateFuture(future));
            }
        }
        return res;
    }

  private:
    typename core::FutureDispatcherTypeSelector<typename SetterFuture::Traits, REQUEST_LIMIT>::Type fSetDispatcher;
};

}  // namespace middleware::core
