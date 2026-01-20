#include "middleware/core/cluster_connection.h"

#include "middleware/core/icluster_connection_configuration_base.h"
#include "middleware/core/types.h"

// suppress misra 5.2.5,5.2.3 EOF: const removal ok. Polymorphic cast ok.
namespace middleware::core
{

ClusterConnectionNoTimeoutProxyOnly::ClusterConnectionNoTimeoutProxyOnly(
    IClusterConnectionConfigurationProxyOnly& configuration)
    : Base(configuration)
{
}

HRESULT
ClusterConnectionNoTimeoutProxyOnly::subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationProxyOnly&>(Base::getConfiguration())
        .subscribe(proxy, serviceInstanceId);
}

void ClusterConnectionNoTimeoutProxyOnly::unsubscribe(ProxyBase& proxy, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationProxyOnly&>(Base::getConfiguration()).unsubscribe(proxy, serviceId);
}

ClusterConnectionNoTimeoutSkeletonOnly::ClusterConnectionNoTimeoutSkeletonOnly(
    IClusterConnectionConfigurationSkeletonOnly& configuration)
    : Base(configuration)
{
}

HRESULT ClusterConnectionNoTimeoutSkeletonOnly::subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationSkeletonOnly&>(Base::getConfiguration())
        .subscribe(skeleton, serviceInstanceId);
}

void ClusterConnectionNoTimeoutSkeletonOnly::unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationSkeletonOnly&>(Base::getConfiguration())
        .unsubscribe(skeleton, serviceId);
}

ClusterConnectionNoTimeoutBidirectional::ClusterConnectionNoTimeoutBidirectional(
    IClusterConnectionConfigurationBidirectional& configuration)
    : Base(configuration)
{
}

HRESULT ClusterConnectionNoTimeoutBidirectional::subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationBidirectional&>(Base::getConfiguration())
        .subscribe(proxy, serviceInstanceId);
}

HRESULT ClusterConnectionNoTimeoutBidirectional::subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationBidirectional&>(Base::getConfiguration())
        .subscribe(skeleton, serviceInstanceId);
}

void ClusterConnectionNoTimeoutBidirectional::unsubscribe(ProxyBase& proxy, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationBidirectional&>(Base::getConfiguration()).unsubscribe(proxy, serviceId);
}

void ClusterConnectionNoTimeoutBidirectional::unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationBidirectional&>(Base::getConfiguration())
        .unsubscribe(skeleton, serviceId);
}

ClusterConnectionBidirectionalWithTimeout::ClusterConnectionBidirectionalWithTimeout(
    IClusterConnectionConfigurationBidirectionalWithTimeout& configuration)
    : Base(configuration)
{
}

HRESULT ClusterConnectionBidirectionalWithTimeout::subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationBidirectionalWithTimeout&>(Base::getConfiguration())
        .subscribe(proxy, serviceInstanceId);
}

void ClusterConnectionBidirectionalWithTimeout::unsubscribe(ProxyBase& proxy, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationBidirectionalWithTimeout&>(Base::getConfiguration())
        .unsubscribe(proxy, serviceId);
}

HRESULT ClusterConnectionBidirectionalWithTimeout::subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationBidirectionalWithTimeout&>(Base::getConfiguration())
        .subscribe(skeleton, serviceInstanceId);
}

void ClusterConnectionBidirectionalWithTimeout::unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationBidirectionalWithTimeout&>(Base::getConfiguration())
        .unsubscribe(skeleton, serviceId);
}

ClusterConnectionProxyOnlyWithTimeout::ClusterConnectionProxyOnlyWithTimeout(
    IClusterConnectionConfigurationProxyOnlyWithTimeout& configuration)
    : Base(configuration)
{
}

HRESULT ClusterConnectionProxyOnlyWithTimeout::subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationProxyOnlyWithTimeout&>(Base::getConfiguration())
        .subscribe(proxy, serviceInstanceId);
}

void ClusterConnectionProxyOnlyWithTimeout::unsubscribe(ProxyBase& proxy, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationProxyOnlyWithTimeout&>(Base::getConfiguration())
        .unsubscribe(proxy, serviceId);
}

ClusterConnectionSkeletonOnlyWithTimeout::ClusterConnectionSkeletonOnlyWithTimeout(
    IClusterConnectionConfigurationSkeletonOnlyWithTimeout& configuration)
    : Base(configuration)
{
}

HRESULT ClusterConnectionSkeletonOnlyWithTimeout::subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId)
{
    return static_cast<IClusterConnectionConfigurationSkeletonOnlyWithTimeout&>(Base::getConfiguration())
        .subscribe(skeleton, serviceInstanceId);
}

void ClusterConnectionSkeletonOnlyWithTimeout::unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId)
{
    static_cast<IClusterConnectionConfigurationSkeletonOnlyWithTimeout&>(Base::getConfiguration())
        .unsubscribe(skeleton, serviceId);
}

}  // namespace middleware::core
