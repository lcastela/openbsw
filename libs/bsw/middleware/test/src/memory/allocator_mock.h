#pragma once

#include <cstdint>

#include <etl/array.h>
#include <gmock/gmock.h>

#include "middleware/core/allocator_base.h"

namespace middleware::memory::test
{

class AllocatorMock : public core::AllocatorBase
{
  public:
    using Base = core::AllocatorBase;
    static constexpr size_t MAX_STORAGE = 4096U;

    AllocatorMock(const AllocatorMock& other) = delete;
    AllocatorMock(AllocatorMock&& other) = delete;
    AllocatorMock& operator=(const AllocatorMock& other) = delete;
    AllocatorMock& operator=(AllocatorMock&& other) = delete;

    MOCK_METHOD(uint8_t*, allocate, (const uint32_t));
    MOCK_METHOD(void, deallocate, (void*));
    MOCK_METHOD(uint8_t*, regionStart, ());
    MOCK_METHOD(bool, isPtrValid, (const void* const));

    static void setAllocatorMock(AllocatorMock& mock);
    static void unsetAllocatorMock();
    static AllocatorMock& getInstance();

  private:
    friend testing::NiceMock<memory::test::AllocatorMock>;

    AllocatorMock();
    ~AllocatorMock();

    volatile uint8_t dummyMutex_{};

    static AllocatorMock* gInstance;
    static etl::array<uint8_t, MAX_STORAGE> gStorage;
};

}  // namespace middleware::memory::test
