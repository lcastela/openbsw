#include "middleware/core/database_manipulator.h"

#include <cstddef>
#include <cstdint>

#include <etl/algorithm.h>
#include <etl/utility.h>
#include <etl/vector.h>

#include "middleware/concurrency/lock_strategies.h"
#include "middleware/core/itransceiver.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/transceiver_container.h"
#include "middleware/core/types.h"

namespace middleware::core::meta
{

HRESULT
DbManipulator::subscribe(middleware::core::meta::TransceiverContainer* const start,
                         middleware::core::meta::TransceiverContainer* const end,
                         ProxyBase& proxy,
                         const InstanceId instanceId,
                         const ServiceId maxServiceId)
{
    auto res = HRESULT::ServiceNotFound;
    MW_SINGLE_CORE_LOCK
    {
        if (proxy.getServiceId() > maxServiceId)
        {
            res = HRESULT::ServiceIdOutOfRange;
        }
        else
        {
            auto const serviceId = proxy.getServiceId();
            auto* containerIterator = getTransceiversByServiceId(start, end, serviceId);
            if (containerIterator != end)
            {
                auto* iter = DbManipulator::findTransceiver(&proxy, *containerIterator->fContainer);
                if (iter != containerIterator->fContainer->end())
                {
                    // order is important - vector must be reordered with new instance id!
                    containerIterator->fContainer->erase(iter);
                    // update instance id
                    proxy.setInstanceId(instanceId);
                    static_cast<void>(containerIterator->fContainer->emplace_back(&proxy));
                    etl::sort(containerIterator->fContainer->begin(),
                              containerIterator->fContainer->end(),
                              TransceiverContainer::TransceiverComparator());
                    res = HRESULT::Ok;
                }
                else
                {
                    if (containerIterator->fContainer->full())
                    {
                        res = HRESULT::TransceiverInitializationFailed;
                    }
                    else
                    {
                        const auto range =
                            getTransceiversByServiceIdAndServiceInstanceId(start, end, serviceId, instanceId);
                        bool addressNotFound = true;
                        while (addressNotFound)
                        {
                            const auto* const iter = etl::find_if(
                                range.first, range.second, [&containerIterator](ITransceiver const* const itrx) {
                                    return (itrx->getAddressId() == containerIterator->fActualAddress);
                                });
                            if (iter == range.second)
                            {
                                addressNotFound = false;
                                proxy.setAddressId(containerIterator->fActualAddress);
                                containerIterator->fActualAddress++;
                            }
                            else
                            {
                                ++containerIterator->fActualAddress;
                            }
                        }
                        proxy.setInstanceId(instanceId);
                        static_cast<void>(containerIterator->fContainer->emplace_back(&proxy));
                        etl::sort(containerIterator->fContainer->begin(),
                                  containerIterator->fContainer->end(),
                                  TransceiverContainer::TransceiverComparator());
                        res = HRESULT::Ok;
                    }
                }
            }
        }
        if (HRESULT::Ok != res)
        {
            proxy.setInstanceId(INVALID_INSTANCE_ID);
        }
    }
    return res;
}

void DbManipulator::unsubscribe(middleware::core::meta::TransceiverContainer* const start,
                                middleware::core::meta::TransceiverContainer* const end,
                                ITransceiver& transceiver,
                                const ServiceId serviceId)
{
    MW_SINGLE_CORE_LOCK
    {
        auto* const containerIterator = getTransceiversByServiceId(start, end, serviceId);
        if (containerIterator != end)
        {
            const auto range = etl::equal_range(containerIterator->fContainer->cbegin(),
                                                containerIterator->fContainer->cend(),
                                                &transceiver,
                                                TransceiverContainer::TransceiverComparator());
            const auto* const iter =
                etl::find_if(range.first, range.second, [&transceiver](ITransceiver const* const itrx) {
                    return (itrx->getAddressId() == transceiver.getAddressId());
                });
            if (iter != containerIterator->fContainer->cend())
            {
                static_cast<void>(containerIterator->fContainer->erase(iter));
                transceiver.setAddressId(INVALID_ADDRESS_ID);
            }
        }
    }
}

HRESULT
DbManipulator::subscribe(middleware::core::meta::TransceiverContainer* const start,
                         middleware::core::meta::TransceiverContainer* const end,
                         SkeletonBase& skeleton,
                         const InstanceId instanceId,
                         const ServiceId maxServiceId)
{
    auto res = HRESULT::ServiceNotFound;
    MW_SINGLE_CORE_LOCK
    {
        if (skeleton.getServiceId() > maxServiceId)
        {
            res = HRESULT::ServiceIdOutOfRange;
        }
        else
        {
            const auto serviceId = skeleton.getServiceId();
            auto* const containerIterator = getTransceiversByServiceId(start, end, serviceId);
            if (containerIterator != end)
            {
                auto* iter = DbManipulator::findTransceiver(&skeleton, *containerIterator->fContainer);
                if (iter != containerIterator->fContainer->end())
                {
                    // order is important - vector must be reordered with new instance id!
                    containerIterator->fContainer->erase(iter);
                    // update instance id
                    skeleton.setInstanceId(instanceId);
                    static_cast<void>(containerIterator->fContainer->emplace_back(&skeleton));
                    etl::sort(containerIterator->fContainer->begin(),
                              containerIterator->fContainer->end(),
                              TransceiverContainer::TransceiverComparator());
                    res = HRESULT::InstanceAlreadyRegistered;
                }
                else
                {
                    // if another skeleton with this serviceInstandId is registered fail
                    if (isSkeletonWithServiceInstanceIdRegistered(*containerIterator->fContainer, instanceId))
                    {
                        res = HRESULT::SkeletonWithThisServiceIdAlreadyRegistered;
                    }
                    else
                    {
                        if (containerIterator->fContainer->full())
                        {
                            res = HRESULT::TransceiverInitializationFailed;
                        }
                        else
                        {
                            skeleton.setInstanceId(instanceId);
                            static_cast<void>(containerIterator->fContainer->emplace_back(&skeleton));
                            etl::sort(containerIterator->fContainer->begin(),
                                      containerIterator->fContainer->end(),
                                      TransceiverContainer::TransceiverComparator());
                            res = HRESULT::Ok;
                        }
                    }
                }
            }
        }
        if ((HRESULT::Ok != res) && (HRESULT::InstanceAlreadyRegistered != res))
        {
            skeleton.setInstanceId(INVALID_INSTANCE_ID);
        }
    }
    return res;
}

TransceiverContainer* DbManipulator::getTransceiversByServiceId(
    middleware::core::meta::TransceiverContainer* const start,
    middleware::core::meta::TransceiverContainer* const end,
    const ServiceId serviceId)
{
    // To avoid code duplication, call const version, then cast away constness
    return const_cast<TransceiverContainer*>(  // NOLINT(cppcoreguidelines-pro-type-const-cast)
        getTransceiversByServiceId(static_cast<const middleware::core::meta::TransceiverContainer*>(start),
                                   static_cast<const middleware::core::meta::TransceiverContainer*>(end),
                                   serviceId));
}

const TransceiverContainer* DbManipulator::getTransceiversByServiceId(
    const middleware::core::meta::TransceiverContainer* const start,
    const middleware::core::meta::TransceiverContainer* const end,
    const ServiceId serviceId)
{
    const auto* const iter =
        etl::lower_bound(start,
                         end,
                         TransceiverContainer{nullptr, serviceId, 0U},
                         [](const TransceiverContainer& lhs, const TransceiverContainer& rhs) -> bool {
                             return lhs.fServiceid < rhs.fServiceid;
                         });
    if ((iter != end) && (iter->fServiceid == serviceId))
    {
        return iter;
    }

    return end;
}

etl::pair<etl::ivector<ITransceiver*>::const_iterator, etl::ivector<ITransceiver*>::const_iterator>
DbManipulator::getTransceiversByServiceIdAndServiceInstanceId(
    const middleware::core::meta::TransceiverContainer* const start,
    const middleware::core::meta::TransceiverContainer* const end,
    const ServiceId serviceId,
    const InstanceId instanceId)
{
    const auto* const transceiversById = getTransceiversByServiceId(start, end, serviceId);
    if (transceiversById != end)
    {
        const internal::DummyTransceiver dummy(instanceId);
        return etl::equal_range(transceiversById->fContainer->cbegin(),
                                transceiversById->fContainer->cend(),
                                &dummy,
                                TransceiverContainer::TransceiverComparatorNoAddressId());
    }

    return etl::make_pair(start->fContainer->cbegin(), start->fContainer->cbegin());
}

ITransceiver* DbManipulator::getSkeletonByServiceIdAndServiceInstanceId(
    const middleware::core::meta::TransceiverContainer* const start,
    const middleware::core::meta::TransceiverContainer* const end,
    const ServiceId serviceId,
    const InstanceId instanceId)
{
    const auto* const transceiversById = getTransceiversByServiceId(start, end, serviceId);
    if (transceiversById != end)
    {
        const internal::DummyTransceiver dummy(instanceId);
        auto const range = etl::equal_range(transceiversById->fContainer->cbegin(),
                                            transceiversById->fContainer->cend(),
                                            &dummy,
                                            TransceiverContainer::TransceiverComparator());
        // there can be only a single skeleton with the same instanceId
        if (range.first != range.second)
        {
            return (*range.first);
        }
    }
    return nullptr;
}

etl::ivector<ITransceiver*>::iterator DbManipulator::findTransceiver(ITransceiver* const& transceiver,
                                                                     etl::ivector<ITransceiver*>& container)
{
    auto* const iter = etl::lower_bound(
        container.begin(), container.end(), transceiver, TransceiverContainer::TransceiverComparator());

    if ((iter != container.cend()) && (*iter)->getInstanceId() == transceiver->getInstanceId() &&
        (*iter)->getAddressId() == transceiver->getAddressId())
    {
        return iter;
    }

    return container.end();
}

bool DbManipulator::isSkeletonWithServiceInstanceIdRegistered(const etl::ivector<ITransceiver*>& container,
                                                              const InstanceId instanceId)
{
    const internal::DummyTransceiver dummy(instanceId);
    auto const range =
        etl::equal_range(container.cbegin(), container.cend(), &dummy, TransceiverContainer::TransceiverComparator());
    return (range.first != range.second);
}

ITransceiver* DbManipulator::getTransceiver(const middleware::core::meta::TransceiverContainer* const start,
                                            const middleware::core::meta::TransceiverContainer* const end,
                                            const ServiceId serviceId,
                                            const InstanceId instanceId,
                                            const uint16_t addressId)
{
    const auto* const containerIterator = getTransceiversByServiceId(start, end, serviceId);
    if (containerIterator != end)
    {
        const internal::DummyTransceiver dummy(instanceId, addressId);
        const auto* const iter = etl::lower_bound(containerIterator->fContainer->cbegin(),
                                                  containerIterator->fContainer->cend(),
                                                  &dummy,
                                                  TransceiverContainer::TransceiverComparator());
        if ((iter != containerIterator->fContainer->cend()) &&
            (!TransceiverContainer::TransceiverComparator()(&dummy,
                                                            *iter)))  // suppress misra 5.14.1: No side effects here.
        {
            return *iter;
        }
    }
    return nullptr;
}

size_t DbManipulator::registeredTransceiversCount(const middleware::core::meta::TransceiverContainer* const start,
                                                  const middleware::core::meta::TransceiverContainer* const end,
                                                  const ServiceId serviceId)
{
    const auto* const containerIterator = getTransceiversByServiceId(start, end, serviceId);
    if (containerIterator != end)
    {
        return containerIterator->fContainer->size();
    }
    return 0U;
}

}  // namespace middleware::core::meta
