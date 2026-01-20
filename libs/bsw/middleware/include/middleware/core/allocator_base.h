#pragma once

#include <cstdint>

#include <etl/delegate.h>

namespace middleware::core
{

class AllocatorBase
{
  public:
    using AllocateFunction = etl::delegate<uint8_t*(const uint32_t)>;
    using DeallocateFunction = etl::delegate<void(void*)>;
    using RegionStartFunction = etl::delegate<uint8_t*()>;
    using PointerValidationFunction = etl::delegate<bool(const void* const)>;

    struct AllocatorStats
    {
        uint32_t allocations;
        uint32_t deallocations;
        uint32_t unknownPtrsError;
    };

    AllocatorBase(volatile uint8_t& mutex,
                  AllocateFunction allocFunction,
                  DeallocateFunction deallocFunction,
                  RegionStartFunction regionStartFunction,
                  PointerValidationFunction ptrValidFunction);

    /**
     * @brief Allocates a unique ownership space of \p payloadSize bytes and returns a pointer to that space's address
     * if successfull, otherwise returns nullptr.
     *
     * @param payloadSize size of the payload to be allocated
     * @return uint8_t* pointer to the allocated payload
     */
    uint8_t* allocate(uint32_t payloadSize);

    /**
     * @brief Allocates a shared ownership space of \p payloadSize + 1 bytes (this byte is written after the payload and
     * represents the \p referenceCounter ) and returns a pointer to that space's address if successfull, otherwise
     * returns nullptr.
     *
     * @param payloadSize size of the payload to be allocated
     * @param referenceCounter number of references that share this payload
     * @return uint8_t* pointer to the allocated payload
     */
    uint8_t* allocateShared(uint32_t payloadSize, uint8_t referenceCounter);

    /**
     * @brief Deallocates the unique ownership space that \p ptr points to and returns a bool with the result of the
     * operation.
     *
     * @param ptr pointer to the payload to be deallocated
     * @return success of the operation
     */
    bool deallocate(uint8_t* ptr);

    /**
     * @brief Deallocates the shared ownership space that \p ptr points to, only if the reference counter (which is
     * located after the payload) is 1, otherwise it simply decrements the reference counter. It returns a bool with the
     * result of the operation.
     *
     * @param ptr pointer to the payload to be deallocated
     * @param payloadSize size of the payload to be deallocated
     * @return success of the operation
     */
    bool deallocateShared(uint8_t* ptr, uint32_t payloadSize);

    /**
     * @brief Returns the starting address of the memory block that is used to allocate additional space in runtime.
     *
     * @return uint8_t* pointer to the start of the memory region
     */
    uint8_t* regionStart() { return regionStart_(); }

    /**
     * @brief Get the statistics of this allocator.
     *
     * @return const AllocatorStats&
     */
    const AllocatorStats& getStats() { return stats_; }

  private:
    volatile uint8_t* mutexPtr_;
    AllocateFunction allocate_;
    DeallocateFunction deallocate_;
    RegionStartFunction regionStart_;
    PointerValidationFunction ptrValid_;
    AllocatorStats stats_;
};

}  // namespace middleware::core
