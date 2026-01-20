#pragma once

#include <etl/array.h>

#include "middleware/core/icluster_connection.h"
#include "middleware/core/instances_database.h"
#include "middleware/core/types.h"

using namespace middleware::core;

class ClusterConnection : public IClusterConnection
{
  public:
    ClusterId getSourceClusterId() const override { return static_cast<ClusterId>(1U); }
    ClusterId getTargetClusterId() const override { return static_cast<ClusterId>(2U); }
    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) override { return HRESULT::Ok; }
    HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) override { return HRESULT::Ok; }
    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) override {}
    void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) override {}
    HRESULT sendMessage(const MiddlewareMessage& msg) const override { return HRESULT::Ok; }
    void processMessage(const MiddlewareMessage& msg) const override {}
    size_t registeredTransceiversCount(const ServiceId serviceId) const override { return 1U; }
    HRESULT dispatchMessage(const MiddlewareMessage& msg) const override { return HRESULT::Ok; }
};

class BadClusterConnection : public IClusterConnection
{
  public:
    ClusterId getSourceClusterId() const override { return static_cast<ClusterId>(1U); }
    ClusterId getTargetClusterId() const override { return static_cast<ClusterId>(2U); }
    HRESULT subscribe(ProxyBase& proxy, const InstanceId serviceInstanceId) override { return HRESULT::NotRegistered; }
    HRESULT subscribe(SkeletonBase& skeleton, const InstanceId serviceInstanceId) override
    {
        return HRESULT::NotRegistered;
    }
    void unsubscribe(ProxyBase& proxy, const ServiceId serviceId) override {}
    void unsubscribe(SkeletonBase& skeleton, const ServiceId serviceId) override {}
    HRESULT sendMessage(const MiddlewareMessage& msg) const override { return HRESULT::NotRegistered; }
    void processMessage(const MiddlewareMessage& msg) const override {}
    size_t registeredTransceiversCount(const ServiceId serviceId) const override { return 0U; }
    HRESULT dispatchMessage(const MiddlewareMessage& msg) const override { return HRESULT::CannotAllocatePayload; }
};

class InstancesDatabase : public ::IInstanceDatabase
{
  public:
    constexpr InstancesDatabase() = default;
    etl::span<IClusterConnection* const> getSkeletonConnectionsRange() const override
    {
        return etl::span(fSkeletonConnections);
    }

    etl::span<IClusterConnection* const> getProxyConnectionsRange() const override
    {
        return etl::span(fProxyConnections);
    }

    etl::span<const ::InstanceId> getInstanceIdsRange() const override { return etl::span(instanceIds_); }

  private:
    const etl::array<const InstanceId, 1U> instanceIds_ = {{1}};
    ClusterConnection clustConn_;
    const etl::array<IClusterConnection* const, 2U> fProxyConnections = {{&clustConn_, nullptr}};
    const etl::array<IClusterConnection* const, 2U> fSkeletonConnections = {{&clustConn_, nullptr}};
};

class BadInstancesDatabase : public ::IInstanceDatabase
{
  public:
    constexpr BadInstancesDatabase() = default;
    etl::span<IClusterConnection* const> getSkeletonConnectionsRange() const override
    {
        return etl::span(fSkeletonConnections);
    }

    etl::span<IClusterConnection* const> getProxyConnectionsRange() const override
    {
        return etl::span(fProxyConnections);
    }

    etl::span<const ::InstanceId> getInstanceIdsRange() const override { return etl::span(instanceIds_); }

  private:
    const etl::array<const InstanceId, 1U> instanceIds_ = {{1}};
    BadClusterConnection clustConn_;
    const etl::array<IClusterConnection* const, 2U> fProxyConnections = {{&clustConn_, nullptr}};
    const etl::array<IClusterConnection* const, 2U> fSkeletonConnections = {{&clustConn_, nullptr}};
};

class EmptyInstancesDatabase : public ::IInstanceDatabase
{
  public:
    constexpr EmptyInstancesDatabase() = default;
    etl::span<IClusterConnection* const> getSkeletonConnectionsRange() const override
    {
        return etl::span(fSkeletonConnections);
    }

    etl::span<IClusterConnection* const> getProxyConnectionsRange() const override
    {
        return etl::span(fProxyConnections);
    }

    etl::span<const ::InstanceId> getInstanceIdsRange() const override { return etl::span(instanceIds_); }

  private:
    const etl::array<const InstanceId, 1U> instanceIds_ = {{1}};
    ClusterConnection clustConn_;
    const etl::array<IClusterConnection* const, 0U> fProxyConnections{};
    const etl::array<IClusterConnection* const, 0U> fSkeletonConnections{};
};

constexpr InstancesDatabase _InstancesDatabase;
constexpr EmptyInstancesDatabase _EmptyInstancesDatabase;
constexpr BadInstancesDatabase _BadInstancesDatabase;
// suppress misra 3.4.1,2.10.5 next_line: Variable must be file local and can be reused.
constexpr etl::array<const ::IInstanceDatabase* const, 1U> INSTANCESDATABASE{&_InstancesDatabase};
constexpr etl::array<const ::IInstanceDatabase* const, 1U> EMPTYINSTANCESDATABASE{&_EmptyInstancesDatabase};
constexpr etl::array<const ::IInstanceDatabase* const, 1U> BADINSTANCESDATABASE{&_BadInstancesDatabase};
