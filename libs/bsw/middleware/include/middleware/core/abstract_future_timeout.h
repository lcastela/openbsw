#pragma once

#include <cstdint>

#include "middleware/core/ifuture.h"

namespace middleware::core
{
class AbstractFutureTimeout : public IFuture
{
  public:
    AbstractFutureTimeout(const AbstractFutureTimeout& other) = delete;
    AbstractFutureTimeout(AbstractFutureTimeout&& other) = delete;
    AbstractFutureTimeout& operator=(const AbstractFutureTimeout& other) = delete;
    AbstractFutureTimeout& operator=(AbstractFutureTimeout&& other) = delete;

    /**
     * @brief Get the timestamp at which this future was used.
     *
     * @return uint32_t
     */
    [[nodiscard]] virtual uint32_t getCallerTimestamp() const = 0;

    /**
     * @brief Set the timestamp at which this future was used.
     *
     * @param timestamp
     */
    virtual void setCallerTimestamp(uint32_t timestamp) = 0;

    /**
     * @brief Method to call when a timeout has occcurred.
     *
     */
    virtual void timeoutExpired() = 0;

  protected:
    AbstractFutureTimeout() = default;
    virtual ~AbstractFutureTimeout() = default;
};

}  // namespace middleware::core
