#include <cstdint>

#include <etl/byte_stream.h>
#include <etl/memory.h>
#include <etl/vector.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "allocator_mock.h"
#include "dsl_logger.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core::test
{

struct SmallTrivialType
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
};

struct BigTrivialType
{
    uint64_t a;
    uint64_t b;
    uint64_t c;
};

struct SmallNonTrivialType
{
    etl::vector<uint8_t, sizeof(SmallTrivialType)> data{};

    struct AllocationPolicy
    {
        static void serialize(const SmallNonTrivialType& obj, etl::span<uint8_t>& buffer)
        {
            etl::byte_stream_writer writer{buffer, etl::endian::native};
            writer.write_unchecked(static_cast<uint32_t>(obj.data.size()));
            for (auto element : obj.data)
            {
                writer.write_unchecked(element);
            }
        }

        static size_t getNeededSize(const SmallNonTrivialType& obj)
        {
            return sizeof(uint32_t) + (obj.data.size() * sizeof(decltype(obj.data)::value_type));
        }

        static SmallNonTrivialType deserialize(const etl::span<uint8_t>& serializedBuffer)
        {
            etl::byte_stream_reader reader{serializedBuffer.begin(), serializedBuffer.end(), etl::endian::native};
            SmallNonTrivialType obj{};
            const uint32_t size = reader.read_unchecked<uint32_t>();
            for (uint32_t idx = 0U; idx < size; ++idx)
            {
                obj.data.push_back(reader.read_unchecked<decltype(obj.data)::value_type>());
            }

            return obj;
        }
    };
};

struct BigNonTrivialType
{
    uint32_t a;
    uint32_t b;
    etl::vector<uint8_t, sizeof(BigTrivialType)> data{};

    struct AllocationPolicy
    {
        static void serialize(const BigNonTrivialType& obj, etl::span<uint8_t>& buffer)
        {
            etl::byte_stream_writer writer{buffer, etl::endian::native};
            writer.write_unchecked(static_cast<uint32_t>(obj.a));
            writer.write_unchecked(static_cast<uint32_t>(obj.b));
            writer.write_unchecked(static_cast<uint32_t>(obj.data.size()));
            for (auto element : obj.data)
            {
                writer.write_unchecked(element);
            }
        }

        static size_t getNeededSize(const BigNonTrivialType& obj)
        {
            return sizeof(obj.a) + sizeof(obj.b) + sizeof(uint32_t) +
                   (obj.data.size() * sizeof(decltype(obj.data)::value_type));
        }

        static BigNonTrivialType deserialize(const etl::span<uint8_t>& serializedBuffer)
        {
            etl::byte_stream_reader reader{serializedBuffer.begin(), serializedBuffer.end(), etl::endian::native};
            BigNonTrivialType obj{};
            obj.a = reader.read_unchecked<decltype(obj.a)>();
            obj.b = reader.read_unchecked<decltype(obj.b)>();
            const uint32_t size = reader.read_unchecked<uint32_t>();
            for (uint32_t idx = 0U; idx < size; ++idx)
            {
                obj.data.push_back(reader.read_unchecked<decltype(obj.data)::value_type>());
            }

            return obj;
        }
    };
};

class AllocatorImpl
{
  public:
    AllocatorImpl() = default;
    ~AllocatorImpl() = default;
    AllocatorImpl(const AllocatorImpl& other) = delete;
    AllocatorImpl(AllocatorImpl&& other) = delete;
    AllocatorImpl& operator=(const AllocatorImpl& other) = delete;
    AllocatorImpl& operator=(AllocatorImpl&& other) = delete;

    template <size_t MAX_COUNT, size_t MAX_SIZE>
    using Storage = etl::vector<etl::array<uint8_t, MAX_SIZE>, MAX_COUNT>;

    enum class PoolId : uint8_t
    {
        Pool32 = 0U,
        Pool64,
        Pool128
    };

    uint8_t* allocate(const uint32_t size)
    {
        uint8_t* res{nullptr};
        if (!pool32_.full())
        {
            if (size <= decltype(pool32_)::value_type::SIZE)
            {
                res = pool32_.emplace_back().data();
            }
        }
        else if (!pool64_.full())
        {
            if (size <= decltype(pool64_)::value_type::SIZE)
            {
                res = pool64_.emplace_back().data();
            }
        }
        else if (!pool128_.full())
        {
            if (size <= decltype(pool128_)::value_type::SIZE)
            {
                res = pool128_.emplace_back().data();
            }
        }
        else
        {
            res = nullptr;
        }

        return res;
    }

    void deallocate(void* ptr)
    {
        if (!pool32_.empty())
        {
            if (reinterpret_cast<decltype(pool32_)::iterator>(ptr) == pool32_.data())
            {
                pool32_.at(0).fill(0U);
                pool32_.clear();
            }
        }
        else if (!pool64_.empty())
        {
            if (reinterpret_cast<decltype(pool64_)::iterator>(ptr) == pool64_.data())
            {
                pool64_.at(0).fill(0U);
                pool64_.clear();
            }
        }
        else if (!pool128_.empty())
        {
            if (reinterpret_cast<decltype(pool128_)::iterator>(ptr) == pool128_.data())
            {
                pool128_.at(0).fill(0U);
                pool128_.clear();
            }
        }
        else
        {
            assert(false);
        }
    }

    [[nodiscard]] bool isAllocatorPoolFull(const PoolId poolId) const
    {
        bool ret = false;

        switch (poolId)
        {
            case PoolId::Pool32:
                ret = pool32_.full();
                break;
            case PoolId::Pool64:
                ret = pool64_.full();
                break;
            case PoolId::Pool128:
                ret = pool128_.full();
                break;
            default:
                break;
        }

        return ret;
    }

    uint8_t* regionStart() { return reinterpret_cast<uint8_t*>(pool32_.data()); }

    bool isPtrValid(const void* const ptr)
    {
        return (ptr == pool32_.data()) || (ptr == pool64_.data()) || (ptr == pool128_.data());
    }

  private:
    Storage<1U, 32U> pool32_;
    Storage<1U, 64U> pool64_;
    Storage<1U, 128U> pool128_;
};

class TestMessageAllocator : public ::testing::Test
{
  public:
    void SetUp() final
    {
        logger_mock_.setup();
        memory::test::AllocatorMock::setAllocatorMock(allocatorMock_);
        ON_CALL(allocatorMock_, allocate).WillByDefault([this](const uint32_t size) -> uint8_t* {
            return allocator_.allocate(size);
        });
        ON_CALL(allocatorMock_, deallocate).WillByDefault([this](void* ptr) -> void { allocator_.deallocate(ptr); });
        ON_CALL(allocatorMock_, regionStart).WillByDefault([this](void) -> uint8_t* {
            return allocator_.regionStart();
        });
        ON_CALL(allocatorMock_, isPtrValid).WillByDefault([this](const void* const ptr) -> bool {
            return allocator_.isPtrValid(ptr);
        });
    }

    void TearDown() final { logger_mock_.teardown(); }

    const AllocatorImpl& getAllocatorImpl() { return allocator_; }

  protected:
    middleware::logger::test::DslLogger logger_mock_{};
    testing::NiceMock<memory::test::AllocatorMock> allocatorMock_{};
    AllocatorImpl allocator_{};
};

TEST_F(TestMessageAllocator, TestSmallTrivialType)
{
    // ARRANGE
    const SmallTrivialType obj{
        0xF1359EA0U,
        0x51314BA1U,
        0x1289CEA2U,
        0x902C37FFU,
    };
    core::MiddlewareMessage msg{};

    // ACT
    core::HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);
    const auto storedObj = MessageAllocator::getInstance().readPayload<SmallTrivialType>(msg);
    MessageAllocator::deallocate(msg);

    // ASSERT
    EXPECT_EQ(ret, core::HRESULT::Ok);
    EXPECT_EQ(obj.a, storedObj.a);
    EXPECT_EQ(obj.b, storedObj.b);
    EXPECT_EQ(obj.c, storedObj.c);
    EXPECT_EQ(obj.d, storedObj.d);
}

TEST_F(TestMessageAllocator, TestBigTrivialType)
{
    // ARRANGE
    const BigTrivialType obj{0xF1359EA0221A3749U, 0x51314BA1F17BCD21U, 0x1289CEA256BD29A4U};
    core::MiddlewareMessage msg{};

    // ACT
    core::HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);
    const auto storedObj = MessageAllocator::getInstance().readPayload<BigTrivialType>(msg);
    MessageAllocator::deallocate(msg);

    // ASSERT
    EXPECT_EQ(ret, core::HRESULT::Ok);
    EXPECT_EQ(obj.a, storedObj.a);
    EXPECT_EQ(obj.b, storedObj.b);
    EXPECT_EQ(obj.c, storedObj.c);
}

TEST_F(TestMessageAllocator, TestSmallNonTrivialType)
{
    // ARRANGE
    SmallNonTrivialType obj{};
    obj.data.push_back(0x31U);
    obj.data.push_back(0x27U);
    obj.data.push_back(0x99U);
    obj.data.push_back(0x50U);
    core::MiddlewareMessage msg{};

    // ACT
    core::HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);
    const auto storedObj = MessageAllocator::getInstance().readPayload<SmallNonTrivialType>(msg);
    MessageAllocator::deallocate(msg);

    // ASSERT
    EXPECT_EQ(ret, core::HRESULT::Ok);
    EXPECT_EQ(obj.data, storedObj.data);
}

TEST_F(TestMessageAllocator, BigNonTrivialType)
{
    // ARRANGE
    BigNonTrivialType obj{};
    obj.a = 0xF131125CU;
    obj.b = 0x5DC09EA0U;
    obj.data.push_back(0xF1U);
    obj.data.push_back(0x21U);
    obj.data.push_back(0x99U);
    obj.data.push_back(0x30U);
    obj.data.push_back(0x32U);
    obj.data.push_back(0xAFU);
    obj.data.push_back(0x9DU);
    obj.data.push_back(0xC0U);
    core::MiddlewareMessage msg{};

    // ACT
    core::HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);
    const auto storedObj = MessageAllocator::getInstance().readPayload<BigNonTrivialType>(msg);
    MessageAllocator::deallocate(msg);

    // ASSERT
    EXPECT_EQ(ret, core::HRESULT::Ok);
    EXPECT_EQ(obj.a, storedObj.a);
    EXPECT_EQ(obj.b, storedObj.b);
    EXPECT_EQ(obj.data, storedObj.data);
}

TEST_F(TestMessageAllocator, TestBigTrivialTypeWithMultipleReferenceCounter)
{
    // ARRANGE
    const BigTrivialType obj{0xF1359EA0221A3749U, 0x51314BA1F17BCD21U, 0x1289CEA256BD29A4U};
    core::MiddlewareMessage msg{};
    const uint8_t numberOfReferences = 5U;

    // ACT
    EXPECT_EQ(MessageAllocator::getInstance().allocate(obj, msg, numberOfReferences), core::HRESULT::Ok);

    // ASSERT
    for (size_t readings = 0U; readings < numberOfReferences; readings++)
    {
        EXPECT_TRUE(getAllocatorImpl().isAllocatorPoolFull(AllocatorImpl::PoolId::Pool32));
        const auto storedObj = MessageAllocator::getInstance().readPayload<BigTrivialType>(msg);
        EXPECT_EQ(obj.a, storedObj.a);
        EXPECT_EQ(obj.b, storedObj.b);
        EXPECT_EQ(obj.c, storedObj.c);
        MessageAllocator::deallocate(msg);
    }
    EXPECT_FALSE(getAllocatorImpl().isAllocatorPoolFull(AllocatorImpl::PoolId::Pool32));
}

TEST_F(TestMessageAllocator, TestBigNonTrivialTypeWithMultipleReferenceCounter)
{
    // ARRANGE
    BigNonTrivialType obj{};
    obj.a = 0xF131125CU;
    obj.b = 0x5DC09EA0U;
    obj.data.push_back(0xF1U);
    obj.data.push_back(0x21U);
    obj.data.push_back(0x99U);
    obj.data.push_back(0x30U);
    obj.data.push_back(0x32U);
    obj.data.push_back(0xAFU);
    obj.data.push_back(0x9DU);
    obj.data.push_back(0xC0U);
    core::MiddlewareMessage msg{};
    const uint8_t numberOfReferences = 5U;

    // ACT
    EXPECT_EQ(MessageAllocator::getInstance().allocate(obj, msg, numberOfReferences), core::HRESULT::Ok);

    // ASSERT
    for (size_t readings = 0U; readings < numberOfReferences; readings++)
    {
        EXPECT_TRUE(getAllocatorImpl().isAllocatorPoolFull(AllocatorImpl::PoolId::Pool32));
        const auto storedObj = MessageAllocator::getInstance().readPayload<BigNonTrivialType>(msg);
        EXPECT_EQ(obj.a, storedObj.a);
        EXPECT_EQ(obj.b, storedObj.b);
        EXPECT_EQ(obj.data, storedObj.data);
        MessageAllocator::deallocate(msg);
    }
    EXPECT_FALSE(getAllocatorImpl().isAllocatorPoolFull(AllocatorImpl::PoolId::Pool32));
}

TEST_F(TestMessageAllocator, TestFailedAllocationForTrivialType)
{
    // ARRANGE
    const BigTrivialType obj{0xF1359EA0221A3749U, 0x51314BA1F17BCD21U, 0x1289CEA256BD29A4U};
    core::MiddlewareMessage msg1{};
    core::MiddlewareMessage msg2{};
    core::MiddlewareMessage msg3{};
    core::MiddlewareMessage msg4{};

    // ACT
    core::HRESULT firstAllocationResult = MessageAllocator::getInstance().allocate(obj, msg1);
    core::HRESULT secondAllocationResult = MessageAllocator::getInstance().allocate(obj, msg2);
    core::HRESULT thirdAllocationResult = MessageAllocator::getInstance().allocate(obj, msg3);

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  logger::Error::Allocation,
                                  HRESULT::CannotAllocatePayload,

                                  msg4.getSourceClusterId(),
                                  msg4.getTargetClusterId(),
                                  msg4.getHeader().serviceId,
                                  msg4.getHeader().serviceInstanceId,
                                  msg4.getHeader().memberId,
                                  msg4.getHeader().requestId,
                                  static_cast<uint32_t>(sizeof(obj)));
    core::HRESULT fourthAllocationResult = MessageAllocator::getInstance().allocate(obj, msg4);

    // ASSERT
    EXPECT_EQ(firstAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(secondAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(thirdAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(fourthAllocationResult, core::HRESULT::CannotAllocatePayload);
    MessageAllocator::deallocate(msg1);
    MessageAllocator::deallocate(msg2);
    MessageAllocator::deallocate(msg3);
}

TEST_F(TestMessageAllocator, TestFailedAllocationForNonTrivialType)
{
    // ARRANGE
    BigNonTrivialType obj{};
    obj.a = 0xF131125CU;
    obj.b = 0x5DC09EA0U;
    obj.data.push_back(0xF1U);
    obj.data.push_back(0x21U);
    obj.data.push_back(0x99U);
    obj.data.push_back(0x30U);
    obj.data.push_back(0x32U);
    obj.data.push_back(0xAFU);
    obj.data.push_back(0x9DU);
    obj.data.push_back(0xC0U);
    core::MiddlewareMessage msg1{};
    core::MiddlewareMessage msg2{};
    core::MiddlewareMessage msg3{};
    core::MiddlewareMessage msg4{};

    // ACT
    core::HRESULT firstAllocationResult = MessageAllocator::getInstance().allocate(obj, msg1);
    core::HRESULT secondAllocationResult = MessageAllocator::getInstance().allocate(obj, msg2);
    core::HRESULT thirdAllocationResult = MessageAllocator::getInstance().allocate(obj, msg3);

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  logger::Error::Allocation,
                                  HRESULT::CannotAllocatePayload,

                                  msg4.getSourceClusterId(),
                                  msg4.getTargetClusterId(),
                                  msg4.getHeader().serviceId,
                                  msg4.getHeader().serviceInstanceId,
                                  msg4.getHeader().memberId,
                                  msg4.getHeader().requestId,
                                  static_cast<uint32_t>(BigNonTrivialType::AllocationPolicy::getNeededSize(obj)));
    core::HRESULT fourthAllocationResult = MessageAllocator::getInstance().allocate(obj, msg4);

    // ASSERT
    EXPECT_EQ(firstAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(secondAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(thirdAllocationResult, core::HRESULT::Ok);
    EXPECT_EQ(fourthAllocationResult, core::HRESULT::CannotAllocatePayload);
    MessageAllocator::deallocate(msg1);
    MessageAllocator::deallocate(msg2);
    MessageAllocator::deallocate(msg3);
}

}  // namespace middleware::core::test
