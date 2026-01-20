#include "middleware/core/skeleton_base.h"

// Library Includes
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <etl/algorithm.h>
#include <etl/span.h>

// Middleware Includes
#include "middleware/concurrency/lock_strategies.h"
#include "middleware/core/icluster_connection.h"
#include "middleware/core/instances_database.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"
#include "middleware/os/task_id_provider.h"

namespace middleware::core
{

HRESULT
SkeletonBase::sendMessage(MiddlewareMessage& msg) const
{
    HRESULT res = HRESULT::ClusterIdNotFoundOrTransceiverNotRegistered;
    const auto* const sender =
        etl::find_if(connections_.begin(), connections_.end(), [&msg](const IClusterConnection* const clusConn) {
            if (clusConn != nullptr)
            {
                return (clusConn->getTargetClusterId() == msg.getTargetClusterId());
            }
            return false;
        });

    if (sender != connections_.end())
    {
        res = (*sender)->sendMessage(msg);
    }
    else
    {
        logger::logMessageSendingFailure(logger::LogLevel::Error, logger::Error::SendMessage, res, msg);
    }

    return res;
}

ClusterId SkeletonBase::getSourceClusterId() const
{
    auto clusterId = static_cast<ClusterId>(INVALID_CLUSTER_ID);
    if (!connections_.empty())
    {
        const auto* const iter =
            etl::find_if(connections_.begin(), connections_.end(), [](const IClusterConnection* const clusConn) {
                return (clusConn != nullptr);
            });

        if (iter != connections_.end())
        {
            clusterId = (*iter)->getSourceClusterId();
        }
    }
    return clusterId;
}

void SkeletonBase::unsubscribe(const ServiceId serviceId)
{
    if (nullptr != connections_.data())
    {
        for (auto* const connection : connections_)
        {
            if (connection != nullptr)
            {
                connection->unsubscribe(*this, serviceId);
            }
        }
    }
    connections_ = etl::span<IClusterConnection*>();
}

const etl::span<IClusterConnection* const>& SkeletonBase::getClusterConnections() const
{
    return connections_;
}

bool SkeletonBase::isInitialized() const
{
    return (!connections_.empty());
}

HRESULT
SkeletonBase::initFromInstancesDatabase(const InstanceId instanceId,
                                        const etl::span<const IInstanceDatabase* const>& dbRange)
{
    unsubscribe(getServiceId());
    const auto* const iter =
        etl::find_if(dbRange.begin(), dbRange.end(), [instanceId](const IInstanceDatabase* const dataBase) -> bool {
            auto const instances = dataBase->getInstanceIdsRange();
            const auto* const instanceIdIt = etl::lower_bound(instances.begin(), instances.end(), instanceId);
            return ((instanceIdIt != instances.end()) && ((*instanceIdIt) == instanceId));
        });
    HRESULT ret = HRESULT::TransceiverInitializationFailed;
    if (iter != dbRange.end())
    {
        auto skeletonCc = (*iter)->getSkeletonConnectionsRange();
        if (skeletonCc.empty())
        {
            instanceId_ = INVALID_INSTANCE_ID;
            ret = HRESULT::NoClientsAvailable;
        }
        else
        {
            bool isRegistered = true;
            for (auto* const clusConn : skeletonCc)
            {
                if (nullptr != clusConn)
                {
                    ret = clusConn->subscribe(*this, instanceId);
                    if ((ret == HRESULT::Ok) || (ret == HRESULT::InstanceAlreadyRegistered))
                    {
                        // suppress misra 6.6.3 next_line: Loop is well formed.
                        continue;
                    }

                    isRegistered = false;
                    break;
                }
            }
            if (isRegistered)
            {
                connections_ = skeletonCc;
            }
            else
            {
                unsubscribe(getServiceId());
                instanceId_ = INVALID_INSTANCE_ID;
                ret = HRESULT::TransceiverInitializationFailed;
            }
        }
    }
    else
    {
        ret = HRESULT::InstanceNotFound;
    }

    if (HRESULT::Ok != ret)
    {
        logger::logInitFailure(logger::LogLevel::Critical,
                               logger::Error::SkeletonInitialization,
                               ret,
                               getServiceId(),
                               instanceId,
                               INVALID_CLUSTER_ID);
    }
    return ret;
}

SkeletonBase::~SkeletonBase() = default;

void SkeletonBase::checkCrossThreadError(const uint32_t initId) const
{
    if (SkeletonBase::isInitialized())
    {
        const auto currentTaskId = ::middleware::os::getProcessId();
        if (initId != currentTaskId)
        {
            // suppress misra 0.1.9 next_line: Statement has side effects in production code
            ::middleware::concurrency::suspendAllInterrupts();

            logger::logCrossThreadViolation(logger::LogLevel::Critical,
                                            logger::Error::SkeletonCrossThreaViolation,
                                            getSourceClusterId(),
                                            getServiceId(),
                                            getInstanceId(),
                                            initId,
                                            currentTaskId);

            // suppress misra 6.2.3,6.3.1 next_line: We want to stay here, forever.
            assert(false);
        }
    }
}

}  // namespace middleware::core
