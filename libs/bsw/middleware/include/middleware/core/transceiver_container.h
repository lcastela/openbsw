#pragma once

#include <cstdint>

#include <etl/utility.h>
#include <etl/vector.h>

#include "middleware/core/itransceiver.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{
namespace meta
{
struct TransceiverContainer
{
    struct TransceiverComparator
    {
        inline bool operator()(const ITransceiver* const lhs, const ITransceiver* const rhs) const
        {
            return (etl::make_pair(lhs->getInstanceId(), lhs->getAddressId()) <
                    etl::make_pair(rhs->getInstanceId(), rhs->getAddressId()));
        }
    };

    struct TransceiverComparatorNoAddressId
    {
        inline bool operator()(const ITransceiver* const lhs, const ITransceiver* const rhs) const
        {
            return (lhs->getInstanceId() < rhs->getInstanceId());
        }
    };

    /* KW_SUPPRESS_START:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */
    TransceiverContainer(const TransceiverContainer&) = delete;
    TransceiverContainer& operator=(const TransceiverContainer&) = delete;
    TransceiverContainer(TransceiverContainer&&) = delete;
    TransceiverContainer& operator=(TransceiverContainer&&) = delete;
    /* KW_SUPPRESS_END:AUTOSAR.ASSIGN.REF_QUAL: IPBD-57903 (Deviation Approved) */

    TransceiverContainer() = delete;
    ~TransceiverContainer() = default;

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)
    /* KW_SUPPRESS_START:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    etl::ivector<ITransceiver*>* const fContainer;
    const ServiceId fServiceid;
    uint16_t fActualAddress;
    /* KW_SUPPRESS_END:MISRA.MEMB.NOT_PRIVATE: IPBD-47241 (Deviation Approved) */
    // NOLINTEND(misc-non-private-member-variables-in-classes): IPBD-47241 (Deviation Approved)
};

}  // namespace meta
}  // namespace core
}  // namespace middleware
