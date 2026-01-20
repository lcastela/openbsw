#pragma once

#include "middleware/core/single_future_dispatcher.h"

namespace middleware::core
{

class SingleTimeoutFutureDispatcher : public SingleFutureDispatcher
{
  public:
    SingleTimeoutFutureDispatcher() = default;

  protected:
    void updateTimeouts(const uint32_t now, const uint32_t timeoutDuration)
    {
        SingleFutureDispatcher::updateTimeouts(now, timeoutDuration);
    }
};

}  // namespace middleware::core
