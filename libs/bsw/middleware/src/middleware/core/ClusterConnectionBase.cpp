#include "middleware/core/ClusterConnectionBase.h"

#include <cstddef>
#include <cstdio>

#include "middleware/core/IClusterConnectionConfigurationBase.h"
#include "middleware/core/ITimeout.h"
#include "middleware/core/LoggerApi.h"
#include "middleware/core/Message.h"
// #include "middleware/core/MessageAllocator.h"
#include "middleware/core/types.h"
#include "middleware/logger/Logger.h"

// suppress misra 5.2.3 EOF: Polymorphic cast ok.
namespace middleware::core
{

ClusterConnectionBase::ClusterConnectionBase(IClusterConnectionConfigurationBase& configuration)
: fConfiguration(configuration)
{}

void ClusterConnectionBase::processMessage(Message const& msg) const
{
    static_cast<void>(dispatchMessage(msg));
    // MessageAllocator::deallocate(msg);
}

// NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
void ClusterConnectionBase::respondWithError(ErrorState const error, Message const& msg) const
{
    Message::Header const& header = msg.getHeader();
    if (header.requestId
        != INVALID_REQUEST_ID) // not a fire/forget method so send error response back
    {
        Message const errorResponse = Message::createErrorResponse(
            header.serviceId,
            header.memberId,
            header.requestId,
            header.serviceInstanceId,
            msg.getHeader().tgtClusterId,
            msg.getHeader().srcClusterId,
            msg.getHeader().addressId,
            error);
        static_cast<void>(sendMessage(errorResponse));
    }
}

uint8_t ClusterConnectionBase::getSourceClusterId() const
{
    return fConfiguration.getSourceClusterId();
}

uint8_t ClusterConnectionBase::getTargetClusterId() const
{
    return fConfiguration.getTargetClusterId();
}

HRESULT // NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
ClusterConnectionBase::sendMessage(Message const& msg) const
{
    auto res = HRESULT::QueueFull;
    if ((msg.getHeader().srcClusterId == msg.getHeader().tgtClusterId))
    {
        res = dispatchMessage(msg);
        // MessageAllocator::deallocate(msg);
    }
    else
    {
        if (fConfiguration.write(msg))
        {
            res = HRESULT::Ok;
        }
        else
        {
            logger::logMessageSendingFailure(
                logger::LogLevel::Error, logger::Error::SendMessage, res, msg);
        }
    }

    return res;
}

size_t ClusterConnectionBase::registeredTransceiversCount(uint16_t const serviceId) const
{
    return fConfiguration.registeredTransceiversCount(serviceId);
}

IClusterConnectionConfigurationBase& ClusterConnectionBase::getConfiguration() const
{
    return fConfiguration;
}

// NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
HRESULT ClusterConnectionBase::dispatchMessage(Message const& msg) const
{
    auto const res = fConfiguration.dispatchMessage(msg);
    if (HRESULT::Ok != res)
    {
        if (HRESULT::ServiceBusy == res)
        {
            // ServiceBusy can only occur when dispatching a get request a message to the Skeleton
            // side
            respondWithError(ErrorState::ServiceBusy, msg);
        }
        else if (HRESULT::ServiceNotFound == res)
        {
            // ServiceNotFound can only occur when dispatching a message to the Skeleton side
            respondWithError(ErrorState::ServiceNotFound, msg);
        }
        else
        {
            // other use cases are not relevant because they won't happen on the message dispatching
        }

        logger::logMessageSendingFailure(
            logger::LogLevel::Error, logger::Error::DispatchMessage, res, msg);
    }

    return res;
}

ClusterConnectionTimeoutBase::ClusterConnectionTimeoutBase(ITimeoutConfiguration& configuration)
: ClusterConnectionBase(configuration)
{}

// suppress misra 5.2.5 next_construct: const removal ok.
void ClusterConnectionTimeoutBase::registerTimeoutTransceiver(ITimeout& transceiver)
{
    static_cast<ITimeoutConfiguration&>((ClusterConnectionBase::getConfiguration()))
        .registerTimeoutTransceiver(transceiver);
}

// suppress misra 5.2.5 next_construct: const removal ok.
void ClusterConnectionTimeoutBase::unregisterTimeoutTransceiver(ITimeout& transceiver)
{
    static_cast<ITimeoutConfiguration&>((ClusterConnectionBase::getConfiguration()))
        .unregisterTimeoutTransceiver(transceiver);
}

// suppress misra 5.2.5 next_construct: const removal ok.
void ClusterConnectionTimeoutBase::updateTimeouts()
{
    static_cast<ITimeoutConfiguration&>((ClusterConnectionBase::getConfiguration()))
        .updateTimeouts();
}

} // namespace middleware::core
