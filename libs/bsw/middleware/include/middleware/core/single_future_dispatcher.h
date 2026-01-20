#pragma once

#include <cstdint>

#include "middleware/core/ifuture.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class SingleFutureDispatcher
{
  public:
    constexpr SingleFutureDispatcher() = default;

    void freeAll();

    IFuture* releaseRequestId(const MiddlewareMessage& msg);

    HRESULT obtainRequestId(uint16_t& requestId, IFuture& future);

    HRESULT invalidateFuture(IFuture& future);

  protected:
    void updateTimeouts(const uint32_t now, const uint32_t timeoutDuration);

  private:
    IFuture* future_{};
    uint16_t actualRequestId_{};
};

}  // namespace middleware::core
