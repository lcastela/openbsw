#include "middleware/core/icluster_connection_configuration_base.h"

#include <etl/algorithm.h>
#include <etl/iterator.h>
#include <etl/vector.h>

#include "middleware/concurrency/lock_strategies.h"
#include "middleware/core/database_manipulator.h"
#include "middleware/core/itimeout.h"
#include "middleware/core/itransceiver.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

// suppress misra 0.1.9 EOF: Print statements are not null statements in production.
namespace middleware::core
{

void ITimeoutConfiguration::registerTimeoutTransceiver(ITimeout& transceiver,
                                                       ::etl::ivector<ITimeout*>& timeoutTransceivers)
{
    MW_SINGLE_CORE_LOCK
    {
        const auto* const iter = etl::find(timeoutTransceivers.cbegin(), timeoutTransceivers.cend(), &transceiver);
        if (iter == timeoutTransceivers.cend())
        {
            if (timeoutTransceivers.full())
            {
                // TODO: is it worth it to log this info?
                // assert();
            }
            else
            {
                timeoutTransceivers.push_back(&transceiver);
            }
        }
    }
}

void ITimeoutConfiguration::unregisterTimeoutTransceiver(ITimeout& transceiver,
                                                         ::etl::ivector<ITimeout*>& timeoutTransceivers)
{
    using ETL_OR_STD::swap;

    MW_SINGLE_CORE_LOCK
    {
        auto* const iter = etl::find(timeoutTransceivers.begin(), timeoutTransceivers.end(), &transceiver);
        if (iter != timeoutTransceivers.cend())
        {
            swap(*iter, timeoutTransceivers.back());
            timeoutTransceivers.pop_back();
        }
        else
        {
            // TODO: is it worth it to log this info?
            // assert();
        }
    }
}

void ITimeoutConfiguration::updateTimeouts(const ::etl::ivector<ITimeout*>& timeoutTransceivers)
{
    for (auto* const transceiver : timeoutTransceivers)
    {
        transceiver->updateTimeouts();
    }
}

HRESULT
IClusterConnectionConfigurationBase::dispatchMessageToProxy(const meta::TransceiverContainer* const proxiesStart,
                                                            const meta::TransceiverContainer* const proxiesEnd,
                                                            const MiddlewareMessage& msg)
{
    HRESULT result = HRESULT::Ok;
    if (msg.isProxyTarget())
    {
        if (msg.isEvent())
        {
            auto const range = meta::DbManipulator::getTransceiversByServiceIdAndServiceInstanceId(
                proxiesStart, proxiesEnd, msg.getHeader().serviceId, msg.getHeader().serviceInstanceId);
            for (const auto* it = range.first; it != range.second; it = etl::next(it))
            {
                static_cast<void>((*it)->onNewMessageReceived(msg));
            }
        }
        else
        {
            const MiddlewareMessage::Header& header = msg.getHeader();
            ITransceiver* const transceiver = meta::DbManipulator::getTransceiver(
                proxiesStart, proxiesEnd, header.serviceId, header.serviceInstanceId, msg.getAddressId());
            if (transceiver != nullptr)
            {
                result = transceiver->onNewMessageReceived(msg);
            }
        }
    }
    else
    {
        result = HRESULT::RoutingError;
    }

    return result;
}

HRESULT
IClusterConnectionConfigurationBase::dispatchMessageToSkeleton(const meta::TransceiverContainer* const skeletonsStart,
                                                               const meta::TransceiverContainer* const skeletonsEnd,
                                                               const MiddlewareMessage& msg)
{
    HRESULT result = HRESULT::Ok;
    if (msg.isProxyTarget())
    {
        result = HRESULT::RoutingError;
    }
    else
    {
        // message comes from proxy
        // dispatch to specific transceiver, identified by serviceInstanceId
        const MiddlewareMessage::Header& header = msg.getHeader();
        auto* const skeleton = meta::DbManipulator::getSkeletonByServiceIdAndServiceInstanceId(
            skeletonsStart, skeletonsEnd, header.serviceId, header.serviceInstanceId);
        if (skeleton != nullptr)
        {
            result = skeleton->onNewMessageReceived(msg);
        }
        else
        {
            result = HRESULT::ServiceNotFound;
        }
    }

    return result;
}

HRESULT IClusterConnectionConfigurationBase::dispatchMessage(const meta::TransceiverContainer* const proxiesStart,
                                                             const meta::TransceiverContainer* const proxiesEnd,
                                                             const meta::TransceiverContainer* const skeletonsStart,
                                                             const meta::TransceiverContainer* const skeletonsEnd,
                                                             const MiddlewareMessage& msg)
{
    HRESULT result = HRESULT::Ok;
    if (msg.isProxyTarget())
    {
        result = dispatchMessageToProxy(proxiesStart, proxiesEnd, msg);
    }
    else
    {
        result = dispatchMessageToSkeleton(skeletonsStart, skeletonsEnd, msg);
    }

    return result;
}

}  // namespace middleware::core
