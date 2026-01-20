#pragma once

#include <cstdint>

#include "middleware/core/types.h"

namespace middleware
{
namespace core
{

class IFuture
{
  public:
    enum class State : uint8_t
    {
        Invalid = 0U,               //!< Never used
        Pending = 1U,               //!< Request in transit
        Ready = 2U,                 //!< Request answered
        Timeout = 3U,               //!< Request timeout
        UserError = 4U,             //!< Request answered with user defined error
        ServiceBusy = 5U,           //!< Service busy (not enough futures in skeleton side)
        ServiceNotFound = 6U,       //!< Service not registered
        SerializationError = 7U,    //!< Could not serialize request or response ( too big, not enough memory )
        DeserializationError = 8U,  //!< Could not deserialize request or response ( too big, not enough memory )
        CouldNotDeliverError = 9U   //!< Message could not be delivered to recipient because of queue full, socket
                                    //!< full or ecu not reachable error.
    };

    IFuture(const IFuture& other) = delete;
    IFuture(IFuture&& other) = delete;
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IFuture& operator=(const IFuture& other) = delete;
    IFuture& operator=(IFuture&& other) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */

    // Getters
    inline State getState() const { return state_; }
    inline RequestId getRequestId() const { return requestId_; }
    inline RequestId getFutureDispatcherSlot() const { return futureDispatcherSlot_; }

    // Setters
    inline void setState(const State state) { state_ = state; }
    inline void setRequestId(const RequestId requestId) { requestId_ = requestId; }
    inline void setFutureDispatcherSlot(const RequestId requestId) { futureDispatcherSlot_ = requestId; }

    // Checkers
    inline bool isReady() const { return (state_ == State::Ready); }
    inline bool hasError() const { return (state_ > State::Ready); }

  protected:
    IFuture() = default;
    ~IFuture() = default;

  private:
    RequestId requestId_{INVALID_REQUEST_ID};
    RequestId futureDispatcherSlot_{INVALID_REQUEST_ID};
    State state_{State::Invalid};
};

}  // namespace core
}  // namespace middleware
