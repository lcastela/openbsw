#include <etl/array.h>
#include <etl/memory.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/allocator_base.h"

namespace middleware::memory::test
{

class TestAllocatorBase : public ::testing::Test
{
  public:
    class AllocatorImpl : public core::AllocatorBase
    {
      public:
        using Base = core::AllocatorBase;

        AllocatorImpl()
            : Base(mutex_,
                   Base::AllocateFunction::create<AllocatorImpl, &AllocatorImpl::doAllocate>(*this),
                   Base::DeallocateFunction::create<AllocatorImpl, &AllocatorImpl::doDeallocate>(*this),
                   Base::RegionStartFunction::create<AllocatorImpl, &AllocatorImpl::getRegionStart>(*this),
                   Base::PointerValidationFunction::create<AllocatorImpl, &AllocatorImpl::isValidPtr>(*this))

        {
        }

        MOCK_METHOD(uint8_t*, doAllocate, (const uint32_t));
        MOCK_METHOD(void, doDeallocate, (void*));
        MOCK_METHOD(bool, isValidPtr, (const void* const));
        MOCK_METHOD(uint8_t*, getRegionStart, ());

      private:
        volatile uint8_t mutex_{};
    };

    TestAllocatorBase() = default;

    void SetUp() final
    {
        ON_CALL(impl_, isValidPtr).WillByDefault(testing::Invoke([this](const void* const ptr) -> bool {
            return ptr == storage_.data();
        }));
        ON_CALL(impl_, getRegionStart).WillByDefault(testing::Return(storage_.begin()));
        ON_CALL(impl_, doAllocate).WillByDefault(testing::Invoke([this](const size_t size) -> uint8_t* {
            return (size <= storage_.max_size()) ? storage_.data() : nullptr;
        }));
        ON_CALL(impl_, doDeallocate).WillByDefault(testing::Invoke([this](void*) -> void { storage_.fill(0U); }));
    }

  protected:
    testing::NiceMock<AllocatorImpl> impl_;
    etl::array<uint8_t, 32U> storage_;
};

TEST_F(TestAllocatorBase, TestSuccessAllocate)
{
    // ARRANGE
    const uint32_t size = 20U;

    // ACT
    uint8_t* externalPtr = impl_.allocate(size);

    // ASSERT
    EXPECT_EQ(externalPtr, storage_.begin());
    EXPECT_EQ(impl_.getStats().allocations, 1U);
}

TEST_F(TestAllocatorBase, TestFailedAllocate)
{
    // ARRANGE
    const uint32_t size = 64U;

    // ACT
    uint8_t* externalPtr = impl_.allocate(size);

    // ASSERT
    EXPECT_EQ(externalPtr, nullptr);
    EXPECT_EQ(impl_.getStats().allocations, 0U);
}

TEST_F(TestAllocatorBase, TestAllocateShared)
{
    // ARRANGE
    const uint32_t size = 20U;
    const uint8_t referenceCounter = 5U;

    // ACT
    uint8_t* externalPtr = impl_.allocateShared(size, referenceCounter);

    // ASSERT
    EXPECT_EQ(externalPtr, storage_.begin());
    EXPECT_EQ(impl_.getStats().allocations, 1U);
    EXPECT_EQ(etl::get_object_at<uint8_t>(etl::next(storage_.data(), size)), 5U);
}

TEST_F(TestAllocatorBase, TestFailedAllocateShared)
{
    // ARRANGE
    const uint32_t size = 64U;
    const uint8_t referenceCounter = 5U;

    // ACT
    uint8_t* externalPtr = impl_.allocateShared(size, referenceCounter);

    // ASSERT
    EXPECT_EQ(externalPtr, nullptr);
    EXPECT_EQ(impl_.getStats().allocations, 0U);
}

TEST_F(TestAllocatorBase, TestDeallocate)
{
    // ARRANGE
    const uint32_t size = 20U;
    uint8_t* const externalPtr = impl_.allocate(size);
    EXPECT_CALL(impl_, doDeallocate).Times(1U);

    // ACT
    impl_.deallocate(externalPtr);

    // ASSERT
    EXPECT_EQ(impl_.getStats().deallocations, 1U);
    EXPECT_EQ(impl_.getStats().unknownPtrsError, 0U);
}

TEST_F(TestAllocatorBase, TestFailedDeallocate)
{
    // ARRANGE
    const uint32_t size = 20U;
    uint8_t* const externalPtr = impl_.allocate(size);
    EXPECT_CALL(impl_, doDeallocate).Times(0U);

    // ACT
    impl_.deallocate(externalPtr + 1U);

    // ASSERT
    EXPECT_EQ(impl_.getStats().deallocations, 0U);
    EXPECT_EQ(impl_.getStats().unknownPtrsError, 1U);
}

TEST_F(TestAllocatorBase, TestDeallocateShared)
{
    // ARRANGE
    const uint32_t size = 20U;
    const uint8_t referenceCounter = 5U;
    uint8_t* const externalPtr = impl_.allocateShared(size, referenceCounter);

    // ACT
    impl_.allocateShared(size, referenceCounter);

    // ASSERT
    for (size_t counter = 1U; counter < referenceCounter; counter++)
    {
        EXPECT_CALL(impl_, doDeallocate).Times(0U);
        impl_.deallocateShared(externalPtr, size);
        EXPECT_EQ(impl_.getStats().deallocations, 0U);
        EXPECT_EQ(impl_.getStats().unknownPtrsError, 0U);
        EXPECT_EQ(etl::get_object_at<uint8_t>(etl::next(storage_.data(), size)), referenceCounter - counter);
    }

    // deallocation should only happen after referenceCounter is 1
    EXPECT_CALL(impl_, doDeallocate).Times(1U);
    impl_.deallocateShared(externalPtr, size);
    EXPECT_EQ(impl_.getStats().deallocations, 1U);
    EXPECT_EQ(impl_.getStats().unknownPtrsError, 0U);
}

TEST_F(TestAllocatorBase, TestFailedDeallocateShared)
{
    // ARRANGE
    const uint32_t size = 64U;
    const uint8_t referenceCounter = 5U;
    uint8_t* const externalPtr = impl_.allocateShared(size, referenceCounter);
    EXPECT_CALL(impl_, doDeallocate).Times(0U);

    // ACT
    impl_.deallocateShared(externalPtr, size);

    // ASSERT
    EXPECT_EQ(impl_.getStats().deallocations, 0U);
    EXPECT_EQ(impl_.getStats().unknownPtrsError, 1U);
}

}  // namespace middleware::memory::test
