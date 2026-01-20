#pragma once

#include <etl/type_traits.h>

#include "middleware/core/future_dispatcher.h"
#include "middleware/core/single_future_dispatcher.h"
#include "middleware/core/timeout_enabled_future_dispatcher.h"

namespace middleware::core
{

template <typename FutureTraits, uint16_t REQUEST_LIMIT, typename Specialization = void>
struct FutureDispatcherTypeSelector;

template <typename FutureTraits>
struct FutureDispatcherTypeSelector<FutureTraits,
                                    1U,
                                    typename etl::enable_if<(FutureTraits::TIMEOUT_VALUE == 0U)>::type>
{
    using Type = SingleFutureDispatcher;
};
// suppress misra 14.7.1 next_lines 5: Templates specialization might be instantiated in production code, or when UT is
// 100%.
/* KW_SUPPRESS_START:MISRA.NS.GLOBAL: IPBD-58092 (False positive) */
template <typename FutureTraits, uint16_t REQUEST_LIMIT>
struct FutureDispatcherTypeSelector<
    FutureTraits,
    REQUEST_LIMIT,
    typename etl::enable_if<(REQUEST_LIMIT > 1U) && (FutureTraits::TIMEOUT_VALUE == 0U)>::type>
{
    using Type = FutureDispatcher<REQUEST_LIMIT>;
};
/* KW_SUPPRESS_END:MISRA.NS.GLOBAL: IPBD-58092 (False positive) */

template <typename FutureTraits, uint16_t REQUEST_LIMIT>
struct FutureDispatcherTypeSelector<FutureTraits,
                                    REQUEST_LIMIT,
                                    typename etl::enable_if<(FutureTraits::TIMEOUT_VALUE != 0U)>::type>
{
    using Type = TimeoutEnabledFutureDispatcher<FutureTraits, REQUEST_LIMIT>;
};

}  // namespace middleware::core
