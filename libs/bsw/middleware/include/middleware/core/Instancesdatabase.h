#pragma once

#include <etl/span.h>

#include "icluster_connection.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{

struct IInstanceDatabase
{
    virtual etl::span<IClusterConnection* const> getSkeletonConnectionsRange() const = 0;

    virtual etl::span<IClusterConnection* const> getProxyConnectionsRange() const = 0;

    virtual etl::span<const InstanceId> getInstanceIdsRange() const = 0;

  private:
    IInstanceDatabase& operator=(const IInstanceDatabase&);
};

}  // namespace core
}  // namespace middleware
