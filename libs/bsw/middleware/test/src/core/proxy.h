#include "gtest/gtest.h"
#include "middleware/core/ProxyBase.h"
#include "middleware/core/types.h"

class ProxyMock : public ::middleware::core::ProxyBase
{
public:
    ProxyMock(
        uint16_t serviceId,
        uint16_t instanceId,
        ::middleware::core::AddressId addressId = etl::numeric_limits<uint16_t>::max())
    : serviceId_(serviceId), ::middleware::core::ProxyBase()
    {
        this->setAddressId(addressId);
        this->setInstanceId(instanceId);
    }

    uint16_t getServiceId() const final { return serviceId_; }

    virtual ::middleware::core::HRESULT onNewMessageReceived(::middleware::core::Message const& msg)
    {
        return ::middleware::core::HRESULT::NotImplemented;
    }

private:
    uint16_t serviceId_;
};
