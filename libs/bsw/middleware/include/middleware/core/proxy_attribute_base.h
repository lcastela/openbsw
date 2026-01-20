#pragma once

#include <etl/type_traits.h>

#include "middleware/core/future_dispatcher_type_selector.h"
#include "middleware/core/ifuture.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

/* KW_SUPPRESS_START:MISRA.NS.GLOBAL: IPBD-58092 (False positive) */
template <uint16_t REQUEST_LIMIT = 1U, typename Future = void>
class ProxyAttributeBase
{
  public:
    HRESULT get(IFuture& future) noexcept { return get(future, Future::Traits::METHOD_MEMBER_ID); }

    void init(ProxyBase* const proxy) { fProxy = proxy; }

    void freeAll() { fGetDispatcher.freeAll(); }

    template <typename T = Future>
    typename etl::enable_if<(T::Traits::TIMEOUT_VALUE > 0U), void>::type updateTimeouts()
    {
        fGetDispatcher.updateTimeouts();
    }

    IFuture* releaseRequestId(const MiddlewareMessage& msg) { return fGetDispatcher.releaseRequestId(msg); }

    HRESULT invalidateFuture(IFuture& future) { return fGetDispatcher.invalidateFuture(future); }

  protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)
    /* KW_SUPPRESS_START:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    ProxyBase* fProxy = nullptr;
    /* KW_SUPPRESS_END:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    // NOLINTEND(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)

    ProxyAttributeBase() = default;

    ~ProxyAttributeBase() = default;

  private:
    typename FutureDispatcherTypeSelector<typename Future::Traits, REQUEST_LIMIT>::Type fGetDispatcher;

    HRESULT get(IFuture& future, const MemberId memberId) noexcept
    {
        HRESULT res = HRESULT::NotRegistered;
        if ((fProxy != nullptr) && fProxy->isInitialized())
        {
            uint16_t requestId = middleware::core::INVALID_REQUEST_ID;
            res = fGetDispatcher.obtainRequestId(requestId, future);
            MiddlewareMessage msg = fProxy->generateMessageHeader(memberId, requestId);
            if (res == HRESULT::Ok)
            {
                res = fProxy->sendMessage(msg);
            }
            else
            {
                logger::logMessageSendingFailure(logger::LogLevel::Error, logger::Error::SendMessage, res, msg);
            }

            if (res != HRESULT::Ok)
            {
                static_cast<void>(fGetDispatcher.invalidateFuture(future));
            }
        }

        return res;
    }
};

template <uint16_t REQUEST_LIMIT>
class ProxyAttributeBase<REQUEST_LIMIT, void>
{
  public:
    void init(ProxyBase* const proxy) { fProxy = proxy; }

  protected:
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)
    /* KW_SUPPRESS_START:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    ProxyBase* fProxy = nullptr;
    /* KW_SUPPRESS_END:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    // NOLINTEND(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)

    ProxyAttributeBase() = default;

    ~ProxyAttributeBase() = default;
};
/* KW_SUPPRESS_END:MISRA.NS.GLOBAL: IPBD-58092 (False positive) */

}  // namespace middleware::core
