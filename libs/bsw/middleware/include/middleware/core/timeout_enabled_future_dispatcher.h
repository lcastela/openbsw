#pragma once

#include <cstdint>

#include <etl/type_traits.h>

#include "middleware/core/abstract_future_timeout.h"
#include "middleware/core/future.h"
#include "middleware/core/ifuture.h"
#include "middleware/core/single_timeout_future_dispatcher.h"
#include "middleware/core/timeout_future_dispatcher_base.h"
#include "middleware/time/system_timer_provider.h"

namespace middleware::core
{

template <typename FutureTraits, uint16_t REQUEST_LIMIT = 1U>
class TimeoutEnabledFutureDispatcher;

template <typename FutureTraits, uint16_t REQUEST_LIMIT>
class TimeoutEnabledFutureDispatcher : public etl::conditional<REQUEST_LIMIT == 1U,
                                                               SingleTimeoutFutureDispatcher,
                                                               TimeoutFutureDispatcherBase<REQUEST_LIMIT>>::type
{
    using FutureType = Future<FutureTraits>;
    using Base = typename etl::conditional<REQUEST_LIMIT == 1U,
                                           SingleTimeoutFutureDispatcher,
                                           TimeoutFutureDispatcherBase<REQUEST_LIMIT>>::type;
    static_assert(etl::is_base_of<IFuture, FutureType>::value, "FATAL_ERROR: Future must derive from IFuture");
    static_assert(etl::is_base_of<IFuture, AbstractFutureTimeout>::value,
                  "FATAL_ERROR: Future must derive from AbstractFutureTimeout");

  public:
    TimeoutEnabledFutureDispatcher() = default;

    HRESULT obtainRequestId(uint16_t& requestId, IFuture& future)
    {
        auto const ret = Base::obtainRequestId(requestId, future);
        if (HRESULT::Ok == ret)
        {
            auto const now = ::middleware::time::getCurrentTimeInMs();
            static_cast<AbstractFutureTimeout&>(future).setCallerTimestamp((now + FutureTraits::TIMEOUT_VALUE));
        }
        return ret;
    }

    void updateTimeouts()
    {
        auto const now = ::middleware::time::getCurrentTimeInMs();
        Base::updateTimeouts(now, FutureTraits::TIMEOUT_VALUE);
    }
};

}  // namespace middleware::core
