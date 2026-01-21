#pragma once

#include "middleware/core/IClusterConnection.h"
#include "middleware/core/IClusterConnectionConfigurationBase.h"
#include "middleware/core/ITimeout.h"
#include "middleware/core/LoggerApi.h"
#include "middleware/core/Message.h"
#include "middleware/core/ProxyBase.h"
#include "middleware/core/SkeletonBase.h"
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
    void processMessage(Message const& msg) const override;

    size_t registeredTransceiversCount(uint16_t const serviceId) const override;

protected:
    IClusterConnectionConfigurationBase& getConfiguration() const;

    explicit ClusterConnectionBase(IClusterConnectionConfigurationBase& configuration);

    /*
     *   get source cluster ID of this cluster connection ( sender cluster ID )
     *
     */
    uint8_t getSourceClusterId() const override;

    /*
     *   get target cluster ID of this cluster connection ( receiver cluster ID )
     *
     */
    uint8_t getTargetClusterId() const override;

    HRESULT sendMessage(Message const& msg) const override;

    HRESULT dispatchMessage(Message const& msg) const override;

private:
    void respondWithError(ErrorState const error, Message const& msg) const;

    IClusterConnectionConfigurationBase& fConfiguration;

    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    ClusterConnectionBase(ClusterConnectionBase const&)            = delete;
    ClusterConnectionBase& operator=(ClusterConnectionBase const&) = delete;
    ClusterConnectionBase(ClusterConnectionBase&&)                 = delete;
    ClusterConnectionBase& operator=(ClusterConnectionBase&&)      = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

class ClusterConnectionTimeoutBase : public ClusterConnectionBase
{
public:
    void registerTimeoutTransceiver(ITimeout& transceiver)
        override; /* KW_SUPPRESS:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */

    void unregisterTimeoutTransceiver(ITimeout& transceiver)
        override; /* KW_SUPPRESS:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */

    void updateTimeouts();

protected:
    explicit ClusterConnectionTimeoutBase(ITimeoutConfiguration& configuration);
};

} // namespace middleware::core
