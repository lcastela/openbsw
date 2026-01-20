#include "middleware/core/proxy_base.h"

// Library includes
#include <cassert>
#include <cstdint>

#include <etl/algorithm.h>
#include <etl/span.h>

// Middleware includes
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
ProxyBase::sendMessage(MiddlewareMessage& msg) const
{
    HRESULT res = HRESULT::NotRegistered;
    if (fConnection != nullptr)
    {
        res = fConnection->sendMessage(msg);
    }

    return res;
}

ClusterId ProxyBase::getSourceClusterId() const
{
    auto clusterId = static_cast<ClusterId>(INVALID_CLUSTER_ID);
    if (fConnection != nullptr)
    {
        clusterId = fConnection->getSourceClusterId();
    }
    return clusterId;
}

HRESULT
ProxyBase::initFromInstancesDatabase(const InstanceId instanceId,
                                     const ClusterId sourceCluster,
                                     const etl::span<const IInstanceDatabase* const>& dbRange)
{
    HRESULT ret = HRESULT::TransceiverInitializationFailed;
    unsubscribe(getServiceId());
    const auto* const iter =
        etl::find_if(dbRange.begin(), dbRange.end(), [instanceId](const IInstanceDatabase* const dataBase) -> bool {
            auto const instances = dataBase->getInstanceIdsRange();
            const auto* const instanceIdIt = etl::lower_bound(instances.begin(), instances.end(), instanceId);
            return ((instanceIdIt != instances.end()) && ((*instanceIdIt) == instanceId));
        });
    if (iter != dbRange.end())
    {
        const auto proxyCc = (*iter)->getProxyConnectionsRange();
        const auto* const ccIt =
            etl::find_if(proxyCc.begin(), proxyCc.end(), [sourceCluster](const IClusterConnection* const clusConn) {
                if (clusConn != nullptr)
                {
                    return (clusConn->getSourceClusterId() == sourceCluster);
                }
                return false;
            });
        if (ccIt != proxyCc.end())
        {
            ret = (*ccIt)->subscribe(*this, instanceId);
            if ((HRESULT::Ok == ret) || (HRESULT::InstanceAlreadyRegistered == ret))
            {
                fConnection = (*ccIt);
            }
        }
    }
    // only print error when configuration allows for it
    if ((HRESULT::Ok != ret) && (!dbRange.empty()))
    {
        logger::logInitFailure(logger::LogLevel::Critical,
                               logger::Error::ProxyInitialization,
                               ret,
                               getServiceId(),
                               instanceId,
                               sourceCluster);
    }
    return ret;
}

void ProxyBase::unsubscribe(const ServiceId serviceId)
{
    if (fConnection != nullptr)
    {
        fConnection->unsubscribe(*this, serviceId);
        fConnection = nullptr;
    }
}

MiddlewareMessage ProxyBase::generateMessageHeader(const MemberId memberId, const RequestId requestId) const
{
    if (INVALID_REQUEST_ID != requestId)
    {
        const MiddlewareMessage::Header header{getServiceId(), memberId, requestId, getInstanceId()};
        return MiddlewareMessage::createRequest(
            header, fConnection->getSourceClusterId(), fConnection->getTargetClusterId(), getAddressId());
    }
    return MiddlewareMessage::createFireAndForgetRequest(getServiceId(),
                                                         memberId,
                                                         getInstanceId(),
                                                         fConnection->getSourceClusterId(),
                                                         fConnection->getTargetClusterId());
}

uint8_t ProxyBase::getAddressId() const
{
    return addressId_;
}

bool ProxyBase::isInitialized() const
{
    return (fConnection != nullptr);
}

ProxyBase::~ProxyBase() = default;

void ProxyBase::setAddressId(const uint8_t addressId)
{
    addressId_ = addressId;
}

void ProxyBase::checkCrossThreadError(const uint32_t initId) const
{
    if (ProxyBase::isInitialized())
    {
        const auto currentTaskId = ::middleware::os::getProcessId();
        if (initId != currentTaskId)
        {
            // suppress misra 0.1.9 next_line: Statement has side effects in production code
            ::middleware::concurrency::suspendAllInterrupts();

            logger::logCrossThreadViolation(logger::LogLevel::Critical,
                                            logger::Error::ProxyCrossThreaViolation,
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
