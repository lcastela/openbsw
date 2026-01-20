#pragma once

#include <cstdint>

#include <etl/span.h>
#include <etl/vector.h>

#include "middleware/core/ifuture.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class FutureDispatcherBase
{
  public:
    FutureDispatcherBase(const FutureDispatcherBase&) = delete;
    FutureDispatcherBase& operator=(const FutureDispatcherBase&) = delete;
    FutureDispatcherBase(FutureDispatcherBase&&) = delete;
    FutureDispatcherBase& operator=(FutureDispatcherBase&&) = delete;

    HRESULT obtainRequestId(uint16_t& requestId, IFuture& future);

    void freeAll();

    IFuture* releaseRequestId(const MiddlewareMessage& msg);

    HRESULT invalidateFuture(IFuture& future);

  protected:
    template <size_t REQUEST_LIMIT>
    FutureDispatcherBase(const etl::span<uint32_t> requestIdBegin, etl::vector<IFuture*, REQUEST_LIMIT>& futures)
        : futures_(futures), kRequestdIds(requestIdBegin)
    {
    }

    ~FutureDispatcherBase();

    void updateTimeouts(const uint32_t now, const uint32_t timeoutDuration, etl::ivector<IFuture*>& futuresToErase);

  private:
    using FutureList = etl::ivector<IFuture*>;

    static uint16_t getPositionOfRightMostUnsetBit_(uint32_t freeBlockValue);

    /*
     *   Non const by design. Obtaining a request ID will render said ID unusable, until it's freed.
     *
     *   \return the request id, or INVALID_REQUEST_ID if maximum ID limit has been reached.
     */
    void freeRequestId_(const uint16_t requestId);

    void obtainRequestId_(const uint16_t requestId);

    /*
     *   Non const by design. Obtaining a request ID will render said ID unusable, until it's freed.
     *
     *   \return the request id, or INVALID_REQUEST_ID if maximum ID limit has been reached.
     */
    uint16_t getRequestId_();

    FutureList& futures_;
    const etl::span<uint32_t> kRequestdIds;
    uint16_t actualRequestId_{};
};

}  // namespace middleware::core
