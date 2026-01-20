#pragma once

#include <etl/array.h>
#include <etl/vector.h>

#include "middleware/core/future_dispatcher_base.h"
#include "middleware/core/ifuture.h"

namespace middleware::core
{

template <uint16_t REQUEST_LIMIT>
class TimeoutFutureDispatcherBase : public FutureDispatcherBase
{
    using Base = FutureDispatcherBase;

  public:
    /* KW_SUPPRESS_START:MISRA.CTOR.BASE: IPBD-57613 (False positive) */
    TimeoutFutureDispatcherBase() : Base(requestIds_, futuresArray_) {}
    /* KW_SUPPRESS_END:MISRA.CTOR.BASE: IPBD-57613 (False positive) */

  protected:
    void updateTimeouts(const uint32_t now, const uint32_t timeoutDuration)
    {
        etl::vector<IFuture*, REQUEST_LIMIT> futuresToErase;
        FutureDispatcherBase::updateTimeouts(now, timeoutDuration, futuresToErase);
    }

  private:
    static_assert(REQUEST_LIMIT > 1U, "REQUEST_LIMIT of FutureDispatcher must be at least 2!");
    /* KW_SUPPRESS_START:AUTOSAR.CTOR.NSDMI_INIT_LIST: IPBD-72720 (False positive) */
    static constexpr size_t VECTOR_SIZE =
        (((REQUEST_LIMIT % (sizeof(uint32_t) * 8U)) == 0U) ? (REQUEST_LIMIT / (sizeof(uint32_t) * 8U))
                                                           : ((REQUEST_LIMIT / (sizeof(uint32_t) * 8U)) + 1U));
    /* KW_SUPPRESS_END:AUTOSAR.CTOR.NSDMI_INIT_LIST: IPBD-72720 (False positive) */
    etl::array<uint32_t, VECTOR_SIZE> requestIds_;
    etl::vector<IFuture*, REQUEST_LIMIT> futuresArray_;
};

}  // namespace middleware::core
