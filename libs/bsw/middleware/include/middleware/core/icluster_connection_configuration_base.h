#pragma once

#include <etl/vector.h>

#include "middleware/core/itimeout.h"
#include "middleware/core/itransceiver.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core
{
class ProxyBase;
class SkeletonBase;

namespace meta
{
struct TransceiverContainer;
}

struct IClusterConnectionConfigurationBase
{
    virtual ClusterId getSourceClusterId() const = 0;

    virtual ClusterId getTargetClusterId() const = 0;

    virtual bool write(const MiddlewareMessage& msg) const = 0;

    virtual size_t registeredTransceiversCount(const ServiceId serviceId) const = 0;

    virtual HRESULT dispatchMessage(const MiddlewareMessage& msg) const = 0;

  protected:
    virtual ~IClusterConnectionConfigurationBase() = default;
    IClusterConnectionConfigurationBase() = default;

    static HRESULT dispatchMessageToProxy(const meta::TransceiverContainer* const proxiesStart,
                                          const meta::TransceiverContainer* const proxiesEnd,
                                          const MiddlewareMessage& msg);

    static HRESULT dispatchMessageToSkeleton(const meta::TransceiverContainer* const skeletonsStart,
                                             const meta::TransceiverContainer* const skeletonsEnd,
                                             const MiddlewareMessage& msg);

    static HRESULT dispatchMessage(const meta::TransceiverContainer* const proxiesStart,
                                   const meta::TransceiverContainer* const proxiesEnd,
                                   const meta::TransceiverContainer* const skeletonsStart,
                                   const meta::TransceiverContainer* const skeletonsEnd,
                                   const MiddlewareMessage& msg);

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBase(const IClusterConnectionConfigurationBase&) = delete;
    IClusterConnectionConfigurationBase& operator=(const IClusterConnectionConfigurationBase&) = delete;
    IClusterConnectionConfigurationBase(IClusterConnectionConfigurationBase&&) = delete;
    IClusterConnectionConfigurationBase& operator=(IClusterConnectionConfigurationBase&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct ITimeoutConfiguration : public IClusterConnectionConfigurationBase
{
    virtual void registerTimeoutTransceiver(ITimeout& transceiver) = 0;

    virtual void unregisterTimeoutTransceiver(ITimeout& transceiver) = 0;

    virtual void updateTimeouts() = 0;

  protected:
    virtual ~ITimeoutConfiguration() = default;
    ITimeoutConfiguration() = default;

    static void registerTimeoutTransceiver(ITimeout& transceiver, ::etl::ivector<ITimeout*>& timeoutTransceivers);

    static void unregisterTimeoutTransceiver(ITimeout& transceiver, ::etl::ivector<ITimeout*>& timeoutTransceivers);

    static void updateTimeouts(const ::etl::ivector<ITimeout*>& timeoutTransceivers);

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    ITimeoutConfiguration(const ITimeoutConfiguration&) = delete;
    ITimeoutConfiguration& operator=(const ITimeoutConfiguration&) = delete;
    ITimeoutConfiguration(ITimeoutConfiguration&&) = delete;
    ITimeoutConfiguration& operator=(ITimeoutConfiguration&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationProxyOnly : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) = 0;

  protected:
    virtual ~IClusterConnectionConfigurationProxyOnly() = default;
    IClusterConnectionConfigurationProxyOnly() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationProxyOnly(const IClusterConnectionConfigurationProxyOnly&) = delete;
    IClusterConnectionConfigurationProxyOnly& operator=(const IClusterConnectionConfigurationProxyOnly&) = delete;
    IClusterConnectionConfigurationProxyOnly(IClusterConnectionConfigurationProxyOnly&&) = delete;
    IClusterConnectionConfigurationProxyOnly& operator=(IClusterConnectionConfigurationProxyOnly&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationSkeletonOnly : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) = 0;

  protected:
    virtual ~IClusterConnectionConfigurationSkeletonOnly() = default;
    IClusterConnectionConfigurationSkeletonOnly() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationSkeletonOnly(const IClusterConnectionConfigurationSkeletonOnly&) = delete;
    IClusterConnectionConfigurationSkeletonOnly& operator=(const IClusterConnectionConfigurationSkeletonOnly&) = delete;
    IClusterConnectionConfigurationSkeletonOnly(IClusterConnectionConfigurationSkeletonOnly&&) = delete;
    IClusterConnectionConfigurationSkeletonOnly& operator=(IClusterConnectionConfigurationSkeletonOnly&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationBidirectional : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) = 0;

  protected:
    virtual ~IClusterConnectionConfigurationBidirectional() = default;
    IClusterConnectionConfigurationBidirectional() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBidirectional(const IClusterConnectionConfigurationBidirectional&) = delete;
    IClusterConnectionConfigurationBidirectional& operator=(const IClusterConnectionConfigurationBidirectional&) =
        delete;
    IClusterConnectionConfigurationBidirectional(IClusterConnectionConfigurationBidirectional&&) = delete;
    IClusterConnectionConfigurationBidirectional& operator=(IClusterConnectionConfigurationBidirectional&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationProxyOnlyWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) = 0;

  protected:
    virtual ~IClusterConnectionConfigurationProxyOnlyWithTimeout() = default;
    IClusterConnectionConfigurationProxyOnlyWithTimeout() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationProxyOnlyWithTimeout(const IClusterConnectionConfigurationProxyOnlyWithTimeout&) =
        delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout& operator=(
        const IClusterConnectionConfigurationProxyOnlyWithTimeout&) = delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout(IClusterConnectionConfigurationProxyOnlyWithTimeout&&) = delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout& operator=(
        IClusterConnectionConfigurationProxyOnlyWithTimeout&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationBidirectionalWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) = 0;

  protected:
    IClusterConnectionConfigurationBidirectionalWithTimeout() = default;
    virtual ~IClusterConnectionConfigurationBidirectionalWithTimeout() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBidirectionalWithTimeout(
        const IClusterConnectionConfigurationBidirectionalWithTimeout&) = delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout& operator=(
        const IClusterConnectionConfigurationBidirectionalWithTimeout&) = delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout(IClusterConnectionConfigurationBidirectionalWithTimeout&&) =
        delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout& operator=(
        IClusterConnectionConfigurationBidirectionalWithTimeout&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationSkeletonOnlyWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) = 0;

  protected:
    virtual ~IClusterConnectionConfigurationSkeletonOnlyWithTimeout() = default;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout() = default;

  private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout(
        const IClusterConnectionConfigurationSkeletonOnlyWithTimeout&) = delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout& operator=(
        const IClusterConnectionConfigurationSkeletonOnlyWithTimeout&) = delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout(IClusterConnectionConfigurationSkeletonOnlyWithTimeout&&) =
        delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout& operator=(
        IClusterConnectionConfigurationSkeletonOnlyWithTimeout&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

}  // namespace middleware::core
