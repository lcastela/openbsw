#pragma once

#include "middleware/core/icluster_connection.h"
#include "middleware/core/icluster_connection_configuration_base.h"
#include "middleware/core/itimeout.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{
class ClusterConnectionBase : public IClusterConnection
{
  public:
    /*
     *   Main message processing function. Releases message's resources at the end of the function
     *
     *   \param msg the message to process.
     *
     */
    void processMessage(const MiddlewareMessage& msg) const override;

    size_t registeredTransceiversCount(const ServiceId serviceId) const override;

  protected:
    IClusterConnectionConfigurationBase& getConfiguration() const;

    explicit ClusterConnectionBase(IClusterConnectionConfigurationBase& configuration);

    /*
     *   get source cluster ID of this cluster connection ( sender cluster ID )
     *
     */
    ClusterId getSourceClusterId() const override;

    /*
     *   get target cluster ID of this cluster connection ( receiver cluster ID )
     *
     */
    ClusterId getTargetClusterId() const override;

    HRESULT sendMessage(const MiddlewareMessage& msg) const override;

    HRESULT dispatchMessage(const MiddlewareMessage& msg) const override;

  private:
    void respondWithError(const ErrorState error, const MiddlewareMessage& msg) const;

    IClusterConnectionConfigurationBase& fConfiguration;

    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    ClusterConnectionBase(const ClusterConnectionBase&) = delete;
    ClusterConnectionBase& operator=(const ClusterConnectionBase&) = delete;
    ClusterConnectionBase(ClusterConnectionBase&&) = delete;
    ClusterConnectionBase& operator=(ClusterConnectionBase&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

class ClusterConnectionTimeoutBase : public ClusterConnectionBase
{
  public:
    void registerTimeoutTransceiver(
        ITimeout& transceiver) override; /* KW_SUPPRESS:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */

    void unregisterTimeoutTransceiver(
        ITimeout& transceiver) override; /* KW_SUPPRESS:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */

    void updateTimeouts();

  protected:
    explicit ClusterConnectionTimeoutBase(ITimeoutConfiguration& configuration);
};

}  // namespace middleware::core
