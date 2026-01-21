#pragma once

#include <cstdint>

#include <etl/span.h>

#include "middleware/core/IClusterConnection.h"
#include "middleware/core/ITransceiver.h"
#include "middleware/core/InstancesDatabase.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class EventSender;

class SkeletonBase : public ITransceiver
{
    friend class EventSender;

public:
    bool isInitialized() const override;
    HRESULT sendMessage(Message& msg) const override;
    uint8_t getSourceClusterId() const final;
    etl::span<IClusterConnection* const> const& getClusterConnections() const;

protected:
    virtual ~SkeletonBase();
    void unsubscribe(uint16_t const serviceId);
    void checkCrossThreadError(uint32_t const initId) const;
    HRESULT initFromInstancesDatabase(
        uint16_t const instanceId, etl::span<IInstanceDatabase const* const> const& dbRange);

    etl::span<IClusterConnection* const> connections_;

private:
    uint8_t getAddressId() const final { return INVALID_ADDRESS_ID; }

    void setAddressId(uint8_t const) final {}

    virtual uint32_t getProcessId() const { return INVALID_TASK_ID; }
};

} // namespace middleware::core
