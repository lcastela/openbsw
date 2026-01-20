#pragma once

#include "middleware/core/itimeout.h"
#include "middleware/core/itransceiver.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class SkeletonBase;
class ProxyBase;

class ITimeoutClusterConnection
{
  public:
    // suppress misra 0.1.8 next_line: No side effects by design.
    virtual void registerTimeoutTransceiver(ITimeout&){};

    // suppress misra 0.1.8 next_line: No side effects by design.
    virtual void unregisterTimeoutTransceiver(ITimeout&){};
};

class IClusterConnection : public ITimeoutClusterConnection
{
  public:
    virtual ClusterId getSourceClusterId() const = 0;

    virtual ClusterId getTargetClusterId() const = 0;

    virtual HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) = 0;

    virtual HRESULT sendMessage(const MiddlewareMessage& msg) const = 0;

    virtual void processMessage(const MiddlewareMessage& msg) const = 0;

    virtual size_t registeredTransceiversCount(const ServiceId serviceId) const = 0;

    virtual HRESULT dispatchMessage(const MiddlewareMessage& msg) const = 0;

    IClusterConnection(const IClusterConnection&) = delete;
    IClusterConnection& operator=(const IClusterConnection&) = delete;
    IClusterConnection(IClusterConnection&&) = delete;
    IClusterConnection& operator=(IClusterConnection&&) = delete;

  protected:
    IClusterConnection() = default;
};

}  // namespace middleware::core
