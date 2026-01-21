#pragma once

#include <cstdint>

#include <etl/utility.h>
#include <etl/vector.h>

#include "middleware/core/ITransceiver.h"
#include "middleware/core/TransceiverContainer.h"
#include "middleware/core/types.h"

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

    virtual HRESULT onNewMessageReceived(Message const&) override { return HRESULT::Ok; }

    virtual uint16_t getServiceId() const override { return INVALID_SERVICE_ID; }

    virtual uint8_t getSourceClusterId() const override { return static_cast<uint8_t>(0xFFU); }

    virtual bool isInitialized() const override { return false; }

    virtual HRESULT sendMessage(Message&) const override { return HRESULT::Ok; }

    virtual uint8_t getAddressId() const override { return fAddressId; }

    virtual void setAddressId(uint8_t const addressId) override { fAddressId = addressId; }

    explicit DummyTransceiver(
        uint16_t const instanceId, uint16_t const addressId = INVALID_ADDRESS_ID)
    : Base(instanceId), fAddressId(addressId)
    {}

    virtual ~DummyTransceiver() = default;

private:
    uint16_t fAddressId;
};

} // namespace internal

class DbManipulator
{
public:
    static HRESULT subscribe(
        middleware::core::meta::TransceiverContainer* const start,
        middleware::core::meta::TransceiverContainer* const end,
        ProxyBase& proxy,
        uint16_t const instanceId,
        uint16_t const maxServiceId);

    static HRESULT subscribe(
        middleware::core::meta::TransceiverContainer* const start,
        middleware::core::meta::TransceiverContainer* const end,
        SkeletonBase& skeleton,
        uint16_t const instanceId,
        uint16_t const maxServiceId);

    static void unsubscribe(
        middleware::core::meta::TransceiverContainer* const start,
        middleware::core::meta::TransceiverContainer* const end,
        ITransceiver& transceiver,
        uint16_t const serviceId);

    static TransceiverContainer* getTransceiversByServiceId(
        middleware::core::meta::TransceiverContainer* const start,
        middleware::core::meta::TransceiverContainer* const end,
        uint16_t const serviceId);

    static TransceiverContainer const* getTransceiversByServiceId(
        middleware::core::meta::TransceiverContainer const* const start,
        middleware::core::meta::TransceiverContainer const* const end,
        uint16_t const serviceId);

    static etl::pair<
        etl::ivector<ITransceiver*>::const_iterator,
        etl::ivector<ITransceiver*>::const_iterator>
    getTransceiversByServiceIdAndServiceInstanceId(
        middleware::core::meta::TransceiverContainer const* const start,
        middleware::core::meta::TransceiverContainer const* const end,
        uint16_t const serviceId,
        uint16_t const instanceId);

    static ITransceiver* getSkeletonByServiceIdAndServiceInstanceId(
        middleware::core::meta::TransceiverContainer const* const start,
        middleware::core::meta::TransceiverContainer const* const end,
        uint16_t const serviceId,
        uint16_t const instanceId);

    static etl::ivector<ITransceiver*>::iterator
    findTransceiver(ITransceiver* const& transceiver, etl::ivector<ITransceiver*>& container);

    static bool isSkeletonWithServiceInstanceIdRegistered(
        etl::ivector<ITransceiver*> const& container, uint16_t const instanceId);

    static ITransceiver* getTransceiver(
        middleware::core::meta::TransceiverContainer const* const start,
        middleware::core::meta::TransceiverContainer const* const end,
        uint16_t const serviceId,
        uint16_t const instanceId,
        uint16_t const addressId);

    static size_t registeredTransceiversCount(
        middleware::core::meta::TransceiverContainer const* const start,
        middleware::core::meta::TransceiverContainer const* const end,
        uint16_t const serviceId);
};
} // namespace meta
} // namespace core
} // namespace middleware
