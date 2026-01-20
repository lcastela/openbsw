#include "gtest/gtest.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/types.h"

class ProxyMock : public ::middleware::core::ProxyBase
{

  public:
    ProxyMock(::middleware::core::ServiceId serviceId,
              ::middleware::core::InstanceId instanceId,
              ::middleware::core::AddressId addressId = etl::numeric_limits<uint16_t>::max())
        : serviceId_(serviceId), ::middleware::core::ProxyBase()
    {
        this->setAddressId(addressId);
        this->setInstanceId(instanceId);
    }
    ::middleware::core::ServiceId getServiceId() const final { return serviceId_; }
    virtual ::middleware::core::HRESULT onNewMessageReceived(const ::middleware::core::MiddlewareMessage& msg)
    {
        return ::middleware::core::HRESULT::NotImplemented;
    }

  private:
    ::middleware::core::ServiceId serviceId_;
};
