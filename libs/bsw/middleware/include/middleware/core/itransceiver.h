#pragma once

#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{

class ITransceiver
{
  public:
    InstanceId getInstanceId() const { return instanceId_; };

    virtual ServiceId getServiceId() const = 0;

    virtual uint8_t getAddressId() const = 0;

    virtual HRESULT onNewMessageReceived(const MiddlewareMessage& msg) = 0;

    void setInstanceId(InstanceId const instanceId) { instanceId_ = instanceId; }

    virtual void setAddressId(const uint8_t addressId) = 0;

    virtual bool isInitialized() const = 0;

    virtual HRESULT sendMessage(MiddlewareMessage& msg) const = 0;
    virtual ClusterId getSourceClusterId() const = 0;

    ITransceiver(const ITransceiver&) = delete;
    ITransceiver& operator=(const ITransceiver&) = delete;
    ITransceiver(ITransceiver&&) = delete;
    ITransceiver& operator=(ITransceiver&&) = delete;

  protected:
    InstanceId instanceId_;

    constexpr explicit ITransceiver(const InstanceId instanceId = INVALID_INSTANCE_ID) : instanceId_(instanceId) {}

    virtual ~ITransceiver() = default;
};

}  // namespace core
}  // namespace middleware
