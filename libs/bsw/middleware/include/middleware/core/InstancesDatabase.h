#pragma once

#include <etl/span.h>

#include "IClusterConnection.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{

struct IInstanceDatabase
{
    virtual etl::span<IClusterConnection* const> getSkeletonConnectionsRange() const = 0;

    virtual etl::span<IClusterConnection* const> getProxyConnectionsRange() const = 0;

    virtual etl::span<uint16_t const> getInstanceIdsRange() const = 0;

private:
    IInstanceDatabase& operator=(IInstanceDatabase const&);
};

} // namespace core
} // namespace middleware
