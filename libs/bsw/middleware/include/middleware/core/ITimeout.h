#pragma once

namespace middleware::core
{
struct ITimeout
{
    virtual void updateTimeouts() = 0;
    ITimeout& operator=(ITimeout const&)
        = delete; /* KW_SUPPRESS:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
};
} // namespace middleware::core
