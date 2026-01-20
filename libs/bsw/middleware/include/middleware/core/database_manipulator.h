#pragma once

#include <cstdint>

#include <etl/utility.h>
#include <etl/vector.h>

#include "itransceiver.h"
#include "middleware/core/types.h"
#include "transceiver_container.h"

namespace middleware
{
namespace core
{
class ProxyBase;
class SkeletonBase;
namespace meta
{

namespace internal
{
struct DummyTransceiver final : public ITransceiver
{
    using Base = ITransceiver;

    virtual HRESULT onNewMessageReceived(const MiddlewareMessage&) override { return HRESULT::Ok; }
    virtual ServiceId getServiceId() const override { return INVALID_SERVICE_ID; }
    virtual ClusterId getSourceClusterId() const override { return static_cast<ClusterId>(0xFFU); }
    virtual bool isInitialized() const override { return false; }

    virtual HRESULT sendMessage(MiddlewareMessage&) const override { return HRESULT::Ok; }

    virtual uint8_t getAddressId() const override { return fAddressId; }

    virtual void setAddressId(const uint8_t addressId) override { fAddressId = addressId; }

    explicit DummyTransceiver(const InstanceId instanceId, const uint16_t addressId = INVALID_ADDRESS_ID)
        : Base(instanceId), fAddressId(addressId)
    {
    }

    virtual ~DummyTransceiver() = default;

  private:
    uint16_t fAddressId;
};

}  // namespace internal

class DbManipulator
{
  public:
    static HRESULT subscribe(middleware::core::meta::TransceiverContainer* const start,
                             middleware::core::meta::TransceiverContainer* const end,
                             ProxyBase& proxy,
                             const InstanceId instanceId,
                             const ServiceId maxServiceId);

    static HRESULT subscribe(middleware::core::meta::TransceiverContainer* const start,
                             middleware::core::meta::TransceiverContainer* const end,
                             SkeletonBase& skeleton,
                             const InstanceId instanceId,
                             const ServiceId maxServiceId);

    static void unsubscribe(middleware::core::meta::TransceiverContainer* const start,
                            middleware::core::meta::TransceiverContainer* const end,
                            ITransceiver& transceiver,
                            const ServiceId serviceId);

    static TransceiverContainer* getTransceiversByServiceId(middleware::core::meta::TransceiverContainer* const start,
                                                            middleware::core::meta::TransceiverContainer* const end,
                                                            const ServiceId serviceId);

    static const TransceiverContainer* getTransceiversByServiceId(
        const middleware::core::meta::TransceiverContainer* const start,
        const middleware::core::meta::TransceiverContainer* const end,
        const ServiceId serviceId);

    static etl::pair<etl::ivector<ITransceiver*>::const_iterator, etl::ivector<ITransceiver*>::const_iterator>
    getTransceiversByServiceIdAndServiceInstanceId(const middleware::core::meta::TransceiverContainer* const start,
                                                   const middleware::core::meta::TransceiverContainer* const end,
                                                   const ServiceId serviceId,
                                                   const InstanceId instanceId);

    static ITransceiver* getSkeletonByServiceIdAndServiceInstanceId(
        const middleware::core::meta::TransceiverContainer* const start,
        const middleware::core::meta::TransceiverContainer* const end,
        const ServiceId serviceId,
        const InstanceId instanceId);

    static etl::ivector<ITransceiver*>::iterator findTransceiver(ITransceiver* const& transceiver,
                                                                 etl::ivector<ITransceiver*>& container);

    static bool isSkeletonWithServiceInstanceIdRegistered(const etl::ivector<ITransceiver*>& container,
                                                          const InstanceId instanceId);

    static ITransceiver* getTransceiver(const middleware::core::meta::TransceiverContainer* const start,
                                        const middleware::core::meta::TransceiverContainer* const end,
                                        const ServiceId serviceId,
                                        const InstanceId instanceId,
                                        const uint16_t addressId);

    static size_t registeredTransceiversCount(const middleware::core::meta::TransceiverContainer* const start,
                                              const middleware::core::meta::TransceiverContainer* const end,
                                              const ServiceId serviceId);
};
}  // namespace meta
}  // namespace core
}  // namespace middleware
