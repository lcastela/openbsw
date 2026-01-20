#pragma once
#include <etl/type_traits.h>

#include "middleware/core/cluster_connection_base.h"
#include "middleware/core/icluster_connection_configuration_base.h"
#include "middleware/core/middleware_message.h"

namespace middleware::core
{
class ProxyBase;
class SkeletonBase;

class ClusterConnectionNoTimeoutProxyOnly final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

  public:
    explicit ClusterConnectionNoTimeoutProxyOnly(IClusterConnectionConfigurationProxyOnly& configuration);

    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase&, const InstanceId) final { return HRESULT::NotImplemented; }

    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(SkeletonBase&, const ServiceId) final {}
};

class ClusterConnectionNoTimeoutSkeletonOnly final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

  public:
    explicit ClusterConnectionNoTimeoutSkeletonOnly(IClusterConnectionConfigurationSkeletonOnly& configuration);

    HRESULT subscribe(ProxyBase&, const InstanceId) final { return HRESULT::NotImplemented; }

    HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(ProxyBase&, const ServiceId) final {}

    void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) final;
};

class ClusterConnectionNoTimeoutBidirectional final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

  public:
    explicit ClusterConnectionNoTimeoutBidirectional(IClusterConnectionConfigurationBidirectional& configuration);

    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) final;

    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) final;

    void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) final;
};

class ClusterConnectionBidirectionalWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

  public:
    explicit ClusterConnectionBidirectionalWithTimeout(
        IClusterConnectionConfigurationBidirectionalWithTimeout& configuration);

    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) final;

    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) final;

    void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) final;
};

class ClusterConnectionProxyOnlyWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

  public:
    explicit ClusterConnectionProxyOnlyWithTimeout(IClusterConnectionConfigurationProxyOnlyWithTimeout& configuration);

    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase&, const InstanceId) final { return HRESULT::NotImplemented; }

    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(SkeletonBase&, const ServiceId) final {}
};

class ClusterConnectionSkeletonOnlyWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

  public:
    explicit ClusterConnectionSkeletonOnlyWithTimeout(
        IClusterConnectionConfigurationSkeletonOnlyWithTimeout& configuration);

    HRESULT subscribe(ProxyBase&, const InstanceId) final { return HRESULT::NotImplemented; }

    HRESULT subscribe(SkeletonBase&, const InstanceId) final;

    void unsubscribe([[maybe_unused]] ProxyBase& proxy, [[maybe_unused]] const ServiceId serviceId) final {}

    void unsubscribe(SkeletonBase&, [[maybe_unused]] const ServiceId) final;
    // suppress misra 0.1.8 next_line: No side effects by design.
};

// suppress misra 14.7.1 next_lines 2: Instantiation of this type should lead in compilation error. As designed.
template <typename T, typename Specialization = void>
struct ClusterConnectionTypeSelector;

// suppress misra 14.7.1 next_lines 2: No use case yet.
template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationProxyOnly, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutProxyOnly;
};

template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationSkeletonOnly, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutSkeletonOnly;
};

template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationBidirectional, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutBidirectional;
};

template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationProxyOnlyWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionProxyOnlyWithTimeout;
};

template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationSkeletonOnlyWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionSkeletonOnlyWithTimeout;
};

template <typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<etl::is_base_of<IClusterConnectionConfigurationBidirectionalWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionBidirectionalWithTimeout;
};

}  // namespace middleware::core
