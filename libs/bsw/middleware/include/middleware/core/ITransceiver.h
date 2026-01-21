#pragma once

#include "middleware/core/Message.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{

class ITransceiver
{
public:
    uint16_t getInstanceId() const { return instanceId_; }

    virtual uint16_t getServiceId() const = 0;

    virtual uint8_t getAddressId() const = 0;

    virtual HRESULT onNewMessageReceived(Message const& msg) = 0;

    void setInstanceId(uint16_t const instanceId) { instanceId_ = instanceId; }

    virtual void setAddressId(uint8_t const addressId) = 0;

    virtual bool isInitialized() const = 0;

    virtual HRESULT sendMessage(Message& msg) const = 0;
    virtual uint8_t getSourceClusterId() const      = 0;

    ITransceiver(ITransceiver const&)            = delete;
    ITransceiver& operator=(ITransceiver const&) = delete;
    ITransceiver(ITransceiver&&)                 = delete;
    ITransceiver& operator=(ITransceiver&&)      = delete;

protected:
    uint16_t instanceId_;

    constexpr explicit ITransceiver(uint16_t const instanceId = INVALID_INSTANCE_ID)
    : instanceId_(instanceId)
    {}

    virtual ~ITransceiver() = default;
};

} // namespace core
} // namespace middleware
