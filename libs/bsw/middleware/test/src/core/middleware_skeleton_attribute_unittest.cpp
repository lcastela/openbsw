#include <stdint.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/future.h"
#include "middleware/core/skeleton_attribute.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware
{
namespace core
{
namespace test
{

namespace internal
{
constexpr MemberId event_id = 0U;
}  // namespace internal

using ArgType = uint8_t;
using GetterFutureTraitsPolicy = FutureTraits<ArgType, 0U, false>;

class DerivedSkeleton : public ::middleware::core::SkeletonBase
{
  public:
    MOCK_METHOD(ServiceId, getServiceId, (), (const override));
    MOCK_METHOD(HRESULT, onNewMessageReceived, (MiddlewareMessage const&), (override));

    DerivedSkeleton() : SkeletonBase(), attribute(*this) {}

    SkeletonAttribute<ArgType, internal::event_id, true> attribute;
};

class SkeletonAttributeTest : public ::testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SkeletonAttributeTest, SendPayload)
{
    // ARRANGE
    DerivedSkeleton skeleton;
    const ArgType payload = 0xFFU;

    // ACT & ASSERT
    EXPECT_EQ(skeleton.attribute.send(payload),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point

    skeleton.attribute.set(skeleton.attribute.get());  // make sure to avoid excessive copying
    EXPECT_EQ(skeleton.attribute.send(),
              HRESULT::NotRegistered);  // no cluster connections nor database defined at this point
}

}  // namespace test
}  // namespace core
}  // namespace middleware
