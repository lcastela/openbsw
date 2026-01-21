#pragma once

#include <etl/type_traits.h>

#include "middleware/core/ClusterConnectionBase.h"
#include "middleware/core/IClusterConnectionConfigurationBase.h"
#include "middleware/core/Message.h"

namespace middleware::core
{
class ProxyBase;
class SkeletonBase;

class ClusterConnectionNoTimeoutProxyOnly final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

public:
    explicit ClusterConnectionNoTimeoutProxyOnly(
        IClusterConnectionConfigurationProxyOnly& configuration);

    HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase&, uint16_t const) final { return HRESULT::NotImplemented; }

    void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(SkeletonBase&, uint16_t const) final {}
};

class ClusterConnectionNoTimeoutSkeletonOnly final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

public:
    explicit ClusterConnectionNoTimeoutSkeletonOnly(
        IClusterConnectionConfigurationSkeletonOnly& configuration);

    HRESULT subscribe(ProxyBase&, uint16_t const) final { return HRESULT::NotImplemented; }

    HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(ProxyBase&, uint16_t const) final {}

    void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) final;
};

class ClusterConnectionNoTimeoutBidirectional final : public ClusterConnectionBase
{
    using Base = ClusterConnectionBase;

public:
    explicit ClusterConnectionNoTimeoutBidirectional(
        IClusterConnectionConfigurationBidirectional& configuration);

    HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) final;

    void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) final;

    void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) final;
};

class ClusterConnectionBidirectionalWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

public:
    explicit ClusterConnectionBidirectionalWithTimeout(
        IClusterConnectionConfigurationBidirectionalWithTimeout& configuration);

    HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase& skeleton, uint16_t const serviceInstanceId) final;

    void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) final;

    void unsubscribe(SkeletonBase& skeleton, uint16_t const serviceId) final;
};

class ClusterConnectionProxyOnlyWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

public:
    explicit ClusterConnectionProxyOnlyWithTimeout(
        IClusterConnectionConfigurationProxyOnlyWithTimeout& configuration);

    HRESULT subscribe(ProxyBase& proxy, uint16_t const serviceInstanceId) final;

    HRESULT subscribe(SkeletonBase&, uint16_t const) final { return HRESULT::NotImplemented; }

    void unsubscribe(ProxyBase& proxy, uint16_t const serviceId) final;

    // suppress misra 0.1.8 next_line: No side effects by design.
    void unsubscribe(SkeletonBase&, uint16_t const) final {}
};

class ClusterConnectionSkeletonOnlyWithTimeout final : public ClusterConnectionTimeoutBase
{
    using Base = ClusterConnectionTimeoutBase;

public:
    explicit ClusterConnectionSkeletonOnlyWithTimeout(
        IClusterConnectionConfigurationSkeletonOnlyWithTimeout& configuration);

    HRESULT subscribe(ProxyBase&, uint16_t const) final { return HRESULT::NotImplemented; }

    HRESULT subscribe(SkeletonBase&, uint16_t const) final;

    void
    unsubscribe([[maybe_unused]] ProxyBase& proxy, [[maybe_unused]] uint16_t const serviceId) final
    {}

    void unsubscribe(SkeletonBase&, [[maybe_unused]] uint16_t const) final;
    // suppress misra 0.1.8 next_line: No side effects by design.
};

// suppress misra 14.7.1 next_lines 2: Instantiation of this type should lead in compilation error.
// As designed.
template<typename T, typename Specialization = void>
struct ClusterConnectionTypeSelector;

// suppress misra 14.7.1 next_lines 2: No use case yet.
template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationProxyOnly, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutProxyOnly;
};

template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationSkeletonOnly, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutSkeletonOnly;
};

template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationBidirectional, T>::value>::type>
{
    using type = ClusterConnectionNoTimeoutBidirectional;
};

template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationProxyOnlyWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionProxyOnlyWithTimeout;
};

template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationSkeletonOnlyWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionSkeletonOnlyWithTimeout;
};

template<typename T>
struct ClusterConnectionTypeSelector<
    T,
    typename etl::enable_if<
        etl::is_base_of<IClusterConnectionConfigurationBidirectionalWithTimeout, T>::value>::type>
{
    using type = ClusterConnectionBidirectionalWithTimeout;
};

} // namespace middleware::core
