#pragma once

#include <cstdint>

#include <etl/span.h>

#include "middleware/core/icluster_connection.h"
#include "middleware/core/instances_database.h"
#include "middleware/core/itransceiver.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class EventSender;

class SkeletonBase : public ITransceiver
{
    friend class EventSender;

  public:
    bool isInitialized() const override;
    HRESULT sendMessage(MiddlewareMessage& msg) const override;
    ClusterId getSourceClusterId() const final;
    const etl::span<IClusterConnection* const>& getClusterConnections() const;

  protected:
    virtual ~SkeletonBase();
    void unsubscribe(const ServiceId serviceId);
    void checkCrossThreadError(const uint32_t initId) const;
    HRESULT initFromInstancesDatabase(const InstanceId instanceId,
                                      const etl::span<const IInstanceDatabase* const>& dbRange);

    etl::span<IClusterConnection* const> connections_;

  private:
    uint8_t getAddressId() const final { return INVALID_ADDRESS_ID; }
    void setAddressId(const uint8_t) final {}
    virtual uint32_t getProcessId() const { return INVALID_TASK_ID; }
};

}  // namespace middleware::core
