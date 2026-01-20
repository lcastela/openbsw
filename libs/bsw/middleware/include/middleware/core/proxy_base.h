#pragma once

#include <cstdint>

#include <etl/span.h>

#include "icluster_connection.h"
#include "instances_database.h"
#include "itransceiver.h"

namespace middleware::core
{

class ProxyBase : public ITransceiver
{
  public:
    void setAddressId(const uint8_t addressId) final;
    uint8_t getAddressId() const final;
    bool isInitialized() const override;

    [[nodiscard]] MiddlewareMessage generateMessageHeader(const MemberId memberId,
                                                          const RequestId requestId = INVALID_REQUEST_ID) const;

    [[nodiscard]] HRESULT sendMessage(MiddlewareMessage& msg) const override;

  protected:
    constexpr ProxyBase() : ITransceiver(), addressId_(INVALID_ADDRESS_ID) {}
    virtual ~ProxyBase();
    ClusterId getSourceClusterId() const final;
    void unsubscribe(const ServiceId serviceId);
    void checkCrossThreadError(const uint32_t initId) const;

    HRESULT initFromInstancesDatabase(const InstanceId instanceId,
                                      const ClusterId sourceCluster,
                                      const etl::span<const IInstanceDatabase* const>& dbRange);

    IClusterConnection* fConnection{nullptr};

  private:
    uint8_t addressId_;
};

}  // namespace middleware::core
