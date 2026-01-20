#include "middleware/core/cluster_connection_base.h"

#include <cstddef>
#include <cstdio>

#include "middleware/core/icluster_connection_configuration_base.h"
#include "middleware/core/itimeout.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"

// suppress misra 5.2.3 EOF: Polymorphic cast ok.
namespace middleware::core
{

ClusterConnectionBase::ClusterConnectionBase(IClusterConnectionConfigurationBase& configuration)
    : fConfiguration(configuration)
{
}

void ClusterConnectionBase::processMessage(const MiddlewareMessage& msg) const
{
    static_cast<void>(dispatchMessage(msg));
    MessageAllocator::deallocate(msg);
}

// NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
void ClusterConnectionBase::respondWithError(const ErrorState error, const MiddlewareMessage& msg) const
{
    const MiddlewareMessage::Header& header = msg.getHeader();
    if (header.requestId != INVALID_REQUEST_ID)  // not a fire/forget method so send error response back
    {
        const MiddlewareMessage errorResponse = MiddlewareMessage::createErrorResponse(
            header, msg.getTargetClusterId(), msg.getSourceClusterId(), msg.getAddressId(), error);
        static_cast<void>(sendMessage(errorResponse));
    }
}

ClusterId ClusterConnectionBase::getSourceClusterId() const
{
    return fConfiguration.getSourceClusterId();
}

ClusterId ClusterConnectionBase::getTargetClusterId() const
{
    return fConfiguration.getTargetClusterId();
}

HRESULT  // NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
ClusterConnectionBase::sendMessage(const MiddlewareMessage& msg) const
{
    auto res = HRESULT::QueueFull;
    if ((msg.getSourceClusterId() == msg.getTargetClusterId()))
    {
        res = dispatchMessage(msg);
        MessageAllocator::deallocate(msg);
    }
    else
    {
        if (fConfiguration.write(msg))
        {
            res = HRESULT::Ok;
        }
        else
        {
            logger::logMessageSendingFailure(logger::LogLevel::Error, logger::Error::SendMessage, res, msg);
        }
    }

    return res;
}

size_t ClusterConnectionBase::registeredTransceiversCount(const ServiceId serviceId) const
{
    return fConfiguration.registeredTransceiversCount(serviceId);
}

IClusterConnectionConfigurationBase& ClusterConnectionBase::getConfiguration() const
{
    return fConfiguration;
}

// NOLINTNEXTLINE(misc-no-recursion): IPBD-54665 (Deviation Approved)
HRESULT ClusterConnectionBase::dispatchMessage(const MiddlewareMessage& msg) const
{
    const auto res = fConfiguration.dispatchMessage(msg);
    if (HRESULT::Ok != res)
    {
        if (HRESULT::ServiceBusy == res)
        {
            // ServiceBusy can only occur when dispatching a get request a message to the Skeleton side
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

        logger::logMessageSendingFailure(logger::LogLevel::Error, logger::Error::DispatchMessage, res, msg);
    }

    return res;
}

ClusterConnectionTimeoutBase::ClusterConnectionTimeoutBase(ITimeoutConfiguration& configuration)
    : ClusterConnectionBase(configuration)
{
}
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
    static_cast<ITimeoutConfiguration&>((ClusterConnectionBase::getConfiguration())).updateTimeouts();
}

}  // namespace middleware::core
