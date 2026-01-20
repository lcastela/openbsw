#pragma once

#include <cstdint>

#include <etl/array.h>
#include <etl/vector.h>

#include "middleware/core/future_dispatcher_base.h"
#include "middleware/core/ifuture.h"

namespace middleware::core
{
template <uint16_t REQUEST_LIMIT = 1U>
class FutureDispatcher;

template <uint16_t REQUEST_LIMIT>
class FutureDispatcher : public FutureDispatcherBase
{
    using Base = FutureDispatcherBase;

  public:
    FutureDispatcher() : Base(requestIds_, futuresArray_) {}

  private:
    static_assert(REQUEST_LIMIT > 1U, "REQUEST_LIMIT of FutureDispatcher must be at least 2!");
    static constexpr size_t VECTOR_SIZE =
        (((REQUEST_LIMIT % (sizeof(uint32_t) * 8U)) == 0U) ? (REQUEST_LIMIT / (sizeof(uint32_t) * 8U))
                                                           : ((REQUEST_LIMIT / (sizeof(uint32_t) * 8U)) + 1U));
    etl::array<uint32_t, VECTOR_SIZE> requestIds_{};
    etl::vector<IFuture*, REQUEST_LIMIT> futuresArray_{};
};

}  // namespace middleware::core
