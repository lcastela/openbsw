#include "middleware/core/MessageAllocator.h"

#include <cstdint>
#include <cstring>

#include <etl/iterator.h>
#include <etl/optional.h>
#include <etl/span.h>

#include "middleware/core/LoggerApi.h"
#include "middleware/core/Message.h"
#include "middleware/core/allocator_base.h"
#include "middleware/core/types.h"
#include "middleware/logger/Logger.h"
#include "middleware/memory/allocator_selector.h"

namespace middleware::core
{

MessageAllocator MessageAllocator::gInstance{};

HRESULT MessageAllocator::allocateTrivialType(
    void const* objPtr, size_t const objSize, Message& msg, uint8_t const numberOfReferences)
{
    HRESULT ret = HRESULT::CannotAllocatePayload;

    AllocatorBase& allocator   = memory::getAllocator(msg.getHeader().serviceId);
    bool const isPayloadShared = (numberOfReferences > 1U);
    uint8_t* const buffer = isPayloadShared ? allocator.allocateShared(objSize, numberOfReferences)
                                            : allocator.allocate(objSize);

    if (buffer != nullptr)
    {
        std::memcpy(buffer, objPtr, objSize);
        AllocatorBase& allocator = memory::getAllocator(msg.getHeader().serviceId);
        auto const offset = static_cast<uint32_t>(etl::distance(allocator.regionStart(), buffer));
        msg.setExternalHandle_({offset, static_cast<uint32_t>(objSize), isPayloadShared});
        ret = HRESULT::Ok;
    }
    else
    {
        logger::logAllocationFailure(
            logger::LogLevel::Error,
            logger::Error::Allocation,
            ret,
            msg,
            static_cast<uint32_t>(objSize));
    }

    return ret;
}

etl::optional<etl::span<uint8_t>> MessageAllocator::allocateNonTrivialType_(
    size_t const payloadSize, Message& msg, uint8_t const numberOfReferences)
{
    etl::optional<etl::span<uint8_t>> ret{};

    AllocatorBase& allocator   = memory::getAllocator(msg.getHeader().serviceId);
    bool const isPayloadShared = (numberOfReferences > 1U);
    uint8_t* const buffer      = isPayloadShared
                                     ? allocator.allocateShared(payloadSize, numberOfReferences)
                                     : allocator.allocate(payloadSize);
    if (buffer != nullptr)
    {
        auto const offset = static_cast<uint32_t>(etl::distance(allocator.regionStart(), buffer));
        msg.setExternalHandle_({offset, static_cast<uint32_t>(payloadSize), isPayloadShared});
        ret.emplace(buffer, payloadSize);
    }
    else
    {
        logger::logAllocationFailure(
            logger::LogLevel::Error,
            logger::Error::Allocation,
            core::HRESULT::CannotAllocatePayload,
            msg,
            static_cast<uint32_t>(payloadSize));
    }

    return ret;
}

uint8_t* MessageAllocator::getAllocatorPointerFromMessage(Message const& msg)
{
    AllocatorBase& allocator         = memory::getAllocator(msg.getHeader().serviceId);
    auto const& handle               = msg.getExternalHandle_();
    uint8_t* const externalBufferPtr = etl::next(allocator.regionStart(), handle.offset);

    return externalBufferPtr;
}

void MessageAllocator::deallocate(Message const& msg)
{
    if (msg.hasExternalPayload())
    {
        uint8_t* externalPtr = getAllocatorPointerFromMessage(msg);
        auto const& handle   = msg.getExternalHandle_();
        bool res             = false;
        if (handle.isPayloadShared)
        {
            res = ::middleware::memory::getAllocator(msg.getHeader().serviceId)
                      .deallocateShared(externalPtr, handle.size);
        }
        else
        {
            res = ::middleware::memory::getAllocator(msg.getHeader().serviceId)
                      .deallocate(externalPtr);
        }

        if (!res)
        {
            logger::logAllocationFailure(
                logger::LogLevel::Error,
                logger::Error::Deallocation,
                core::HRESULT::CannotDeallocatePayload,
                msg,
                handle.size);
        }
    }
}

} // namespace middleware::core
