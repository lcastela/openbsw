#pragma once

#include <etl/vector.h>

#include "middleware/core/ITimeout.h"
#include "middleware/core/ITransceiver.h"
#include "middleware/core/Message.h"
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
    virtual uint8_t getSourceClusterId() const = 0;

    virtual uint8_t getTargetClusterId() const = 0;

    virtual bool write(Message const& msg) const = 0;

    virtual size_t registeredTransceiversCount(uint16_t const serviceId) const = 0;

    virtual HRESULT dispatchMessage(Message const& msg) const = 0;

protected:
    virtual ~IClusterConnectionConfigurationBase() = default;
    IClusterConnectionConfigurationBase()          = default;

    static HRESULT dispatchMessageToProxy(
        meta::TransceiverContainer const* const proxiesStart,
        meta::TransceiverContainer const* const proxiesEnd,
        Message const& msg);

    static HRESULT dispatchMessageToSkeleton(
        meta::TransceiverContainer const* const skeletonsStart,
        meta::TransceiverContainer const* const skeletonsEnd,
        Message const& msg);

    static HRESULT dispatchMessage(
        meta::TransceiverContainer const* const proxiesStart,
        meta::TransceiverContainer const* const proxiesEnd,
        meta::TransceiverContainer const* const skeletonsStart,
        meta::TransceiverContainer const* const skeletonsEnd,
        Message const& msg);

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBase(IClusterConnectionConfigurationBase const&) = delete;
    IClusterConnectionConfigurationBase& operator=(IClusterConnectionConfigurationBase const&)
        = delete;
    IClusterConnectionConfigurationBase(IClusterConnectionConfigurationBase&&)            = delete;
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
    ITimeoutConfiguration()          = default;

    static void registerTimeoutTransceiver(
        ITimeout& transceiver, ::etl::ivector<ITimeout*>& timeoutTransceivers);

    static void unregisterTimeoutTransceiver(
        ITimeout& transceiver, ::etl::ivector<ITimeout*>& timeoutTransceivers);

    static void updateTimeouts(::etl::ivector<ITimeout*> const& timeoutTransceivers);

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    ITimeoutConfiguration(ITimeoutConfiguration const&)            = delete;
    ITimeoutConfiguration& operator=(ITimeoutConfiguration const&) = delete;
    ITimeoutConfiguration(ITimeoutConfiguration&&)                 = delete;
    ITimeoutConfiguration& operator=(ITimeoutConfiguration&&)      = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationProxyOnly : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) = 0;

protected:
    virtual ~IClusterConnectionConfigurationProxyOnly() = default;
    IClusterConnectionConfigurationProxyOnly()          = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationProxyOnly(IClusterConnectionConfigurationProxyOnly const&)
        = delete;
    IClusterConnectionConfigurationProxyOnly&
    operator=(IClusterConnectionConfigurationProxyOnly const&)
        = delete;
    IClusterConnectionConfigurationProxyOnly(IClusterConnectionConfigurationProxyOnly&&) = delete;
    IClusterConnectionConfigurationProxyOnly& operator=(IClusterConnectionConfigurationProxyOnly&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationSkeletonOnly : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) = 0;

protected:
    virtual ~IClusterConnectionConfigurationSkeletonOnly() = default;
    IClusterConnectionConfigurationSkeletonOnly()          = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationSkeletonOnly(IClusterConnectionConfigurationSkeletonOnly const&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnly&
    operator=(IClusterConnectionConfigurationSkeletonOnly const&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnly(IClusterConnectionConfigurationSkeletonOnly&&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnly&
    operator=(IClusterConnectionConfigurationSkeletonOnly&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationBidirectional : public IClusterConnectionConfigurationBase
{
    virtual HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) = 0;

protected:
    virtual ~IClusterConnectionConfigurationBidirectional() = default;
    IClusterConnectionConfigurationBidirectional()          = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBidirectional(
        IClusterConnectionConfigurationBidirectional const&)
        = delete;
    IClusterConnectionConfigurationBidirectional&
    operator=(IClusterConnectionConfigurationBidirectional const&)
        = delete;
    IClusterConnectionConfigurationBidirectional(IClusterConnectionConfigurationBidirectional&&)
        = delete;
    IClusterConnectionConfigurationBidirectional&
    operator=(IClusterConnectionConfigurationBidirectional&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationProxyOnlyWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) = 0;

protected:
    virtual ~IClusterConnectionConfigurationProxyOnlyWithTimeout() = default;
    IClusterConnectionConfigurationProxyOnlyWithTimeout()          = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationProxyOnlyWithTimeout(
        IClusterConnectionConfigurationProxyOnlyWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout&
    operator=(IClusterConnectionConfigurationProxyOnlyWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout(
        IClusterConnectionConfigurationProxyOnlyWithTimeout&&)
        = delete;
    IClusterConnectionConfigurationProxyOnlyWithTimeout&
    operator=(IClusterConnectionConfigurationProxyOnlyWithTimeout&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationBidirectionalWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) = 0;

    virtual HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) = 0;

protected:
    IClusterConnectionConfigurationBidirectionalWithTimeout()          = default;
    virtual ~IClusterConnectionConfigurationBidirectionalWithTimeout() = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationBidirectionalWithTimeout(
        IClusterConnectionConfigurationBidirectionalWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout&
    operator=(IClusterConnectionConfigurationBidirectionalWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout(
        IClusterConnectionConfigurationBidirectionalWithTimeout&&)
        = delete;
    IClusterConnectionConfigurationBidirectionalWithTimeout&
    operator=(IClusterConnectionConfigurationBidirectionalWithTimeout&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

struct IClusterConnectionConfigurationSkeletonOnlyWithTimeout : public ITimeoutConfiguration
{
    virtual HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) = 0;

    virtual void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) = 0;

protected:
    virtual ~IClusterConnectionConfigurationSkeletonOnlyWithTimeout() = default;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout()          = default;

private:
    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout(
        IClusterConnectionConfigurationSkeletonOnlyWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout&
    operator=(IClusterConnectionConfigurationSkeletonOnlyWithTimeout const&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout(
        IClusterConnectionConfigurationSkeletonOnlyWithTimeout&&)
        = delete;
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout&
    operator=(IClusterConnectionConfigurationSkeletonOnlyWithTimeout&&)
        = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};

} // namespace middleware::core
