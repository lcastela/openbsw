#include <cassert>

#include "allocator_mock.h"
#include "middleware/core/allocator_base.h"
#include "middleware/core/types.h"
#include "middleware/memory/allocator_selector.h"

namespace middleware::memory
{

namespace test
{

AllocatorMock* AllocatorMock::gInstance{nullptr};
etl::array<uint8_t, AllocatorMock::MAX_STORAGE> AllocatorMock::gStorage{};

AllocatorMock& AllocatorMock::getInstance()
{
    if (gInstance == nullptr)
    {
        assert(false);
    }

    return *gInstance;
}

void AllocatorMock::setAllocatorMock(AllocatorMock& mock)
{
    gInstance = &mock;
    ON_CALL(*gInstance, allocate).WillByDefault([](const uint32_t size) -> uint8_t* {
        if (size > gStorage.max_size())
        {
            assert(false);
        }
        return gStorage.data();
    });
    ON_CALL(*gInstance, deallocate).WillByDefault([](void* ptr) -> void {
        if (ptr != gStorage.data())
        {
            assert(false);
        }
        gStorage.fill(0U);
    });
    ON_CALL(*gInstance, regionStart).WillByDefault([](void) -> uint8_t* { return gStorage.data(); });
    ON_CALL(*gInstance, isPtrValid).WillByDefault([](const void* const ptr) -> bool { return ptr == gStorage.data(); });
}

void AllocatorMock::unsetAllocatorMock()
{
    gInstance = nullptr;
}

AllocatorMock::AllocatorMock()
    : Base(dummyMutex_,
           Base::AllocateFunction::create<AllocatorMock, &AllocatorMock::allocate>(*this),
           Base::DeallocateFunction::create<AllocatorMock, &AllocatorMock::deallocate>(*this),
           Base::RegionStartFunction::create<AllocatorMock, &AllocatorMock::regionStart>(*this),
           Base::PointerValidationFunction::create<AllocatorMock, &AllocatorMock::isPtrValid>(*this))
{
}

AllocatorMock::~AllocatorMock()
{
    unsetAllocatorMock();
}

}  // namespace test

core::AllocatorBase& getAllocator(const core::ServiceId /*unused*/)
{
    return test::AllocatorMock::getInstance();
}

}  // namespace middleware::memory
