#pragma once

#include "middleware/core/ITimeout.h"
#include "middleware/core/ITransceiver.h"
#include "middleware/core/Message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class SkeletonBase;
class ProxyBase;

class ITimeoutClusterConnection
{
public:
    // suppress misra 0.1.8 next_line: No side effects by design.
    virtual void registerTimeoutTransceiver(ITimeout&) {}

    // suppress misra 0.1.8 next_line: No side effects by design.
    virtual void unregisterTimeoutTransceiver(ITimeout&) {}
};

class IClusterConnection : public ITimeoutClusterConnection
{
public:
    virtual uint8_t getSourceClusterId() const = 0;

    virtual uint8_t getTargetClusterId() const = 0;

    virtual HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) = 0;

    virtual HRESULT sendMessage(Message const& msg) const = 0;

    virtual void processMessage(Message const& msg) const = 0;

    virtual size_t registeredTransceiversCount(uint16_t const serviceId) const = 0;

    virtual HRESULT dispatchMessage(Message const& msg) const = 0;

    IClusterConnection(IClusterConnection const&)            = delete;
    IClusterConnection& operator=(IClusterConnection const&) = delete;
    IClusterConnection(IClusterConnection&&)                 = delete;
    IClusterConnection& operator=(IClusterConnection&&)      = delete;

protected:
    IClusterConnection() = default;
};

} // namespace middleware::core
