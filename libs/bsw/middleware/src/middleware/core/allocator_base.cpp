#include "middleware/core/allocator_base.h"

#include <cstdint>

#include <etl/iterator.h>
#include <etl/memory.h>

#include "middleware/concurrency/lock_strategies.h"

namespace middleware::core
{

AllocatorBase::AllocatorBase(volatile uint8_t& mutex,
                             AllocateFunction allocFunction,
                             DeallocateFunction deallocFunction,
                             RegionStartFunction regionStartFunction,
                             PointerValidationFunction ptrValidFunction)
    : mutexPtr_(&mutex),
      allocate_(allocFunction),
      deallocate_(deallocFunction),
      regionStart_(regionStartFunction),
      ptrValid_(ptrValidFunction),
      stats_()
{
}

uint8_t* AllocatorBase::allocate(uint32_t payloadSize)
{
    const middleware::concurrency::ScopedECULock lockElement(mutexPtr_);
    uint8_t* externalPtr = allocate_(payloadSize);
    if (externalPtr != nullptr)
    {
        stats_.allocations++;
    }

    return externalPtr;
}

uint8_t* AllocatorBase::allocateShared(uint32_t payloadSize, uint8_t referenceCounter)
{
    const middleware::concurrency::ScopedECULock lockElement(mutexPtr_);
    uint8_t* externalPtr = allocate_(payloadSize + sizeof(referenceCounter));
    if (externalPtr != nullptr)
    {
        stats_.allocations++;
        etl::construct_object_at<uint8_t>(etl::next(externalPtr, payloadSize), referenceCounter);
    }

    return externalPtr;
}

bool AllocatorBase::deallocate(uint8_t* ptr)
{
    const middleware::concurrency::ScopedECULock lockElement(mutexPtr_);
    bool res = true;
    if (ptrValid_(ptr))
    {
        deallocate_(ptr);
        stats_.deallocations++;
    }
    else
    {
        stats_.unknownPtrsError++;
        res = false;
    }
    return res;
}

bool AllocatorBase::deallocateShared(uint8_t* ptr, uint32_t payloadSize)
{
    const middleware::concurrency::ScopedECULock lockElement(mutexPtr_);
    bool res = true;
    if (ptrValid_(ptr))
    {
        auto& referenceCounter = etl::get_object_at<uint8_t>(etl::next(ptr, payloadSize));
        if (referenceCounter > 1U)
        {
            referenceCounter--;
        }
        else
        {
            deallocate_(ptr);
            stats_.deallocations++;
        }
    }
    else
    {
        stats_.unknownPtrsError++;
        res = false;
    }
    return res;
}

}  // namespace middleware::core
