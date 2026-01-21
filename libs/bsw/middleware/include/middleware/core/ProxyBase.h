#pragma once

#include <cstdint>

#include <etl/span.h>

#include "IClusterConnection.h"
#include "ITransceiver.h"
#include "InstancesDatabase.h"

namespace middleware::core
{

class ProxyBase : public ITransceiver
{
public:
    void setAddressId(uint8_t const addressId) final;
    uint8_t getAddressId() const final;
    bool isInitialized() const override;

    [[nodiscard]] Message generateMessageHeader(
        uint16_t const memberId, uint16_t const requestId = INVALID_REQUEST_ID) const;

    [[nodiscard]] HRESULT sendMessage(Message& msg) const override;

protected:
    constexpr ProxyBase() : ITransceiver(), addressId_(INVALID_ADDRESS_ID) {}

    virtual ~ProxyBase() = default;

    uint8_t getSourceClusterId() const final;
    void unsubscribe(uint16_t const serviceId);
    void checkCrossThreadError(uint32_t const initId) const;

    HRESULT initFromInstancesDatabase(
        uint16_t const instanceId,
        uint8_t const sourceCluster,
        etl::span<IInstanceDatabase const* const> const& dbRange);

    IClusterConnection* fConnection{nullptr};

private:
    uint8_t addressId_;
};

} // namespace middleware::core
