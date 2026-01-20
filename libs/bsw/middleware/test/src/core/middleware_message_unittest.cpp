#include "middleware/core/middleware_message.h"

#include <etl/limits.h>
#include <gtest/gtest.h>

#include "allocator_mock.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/types.h"

namespace middleware::core::test
{

class MiddlewareMessageTest : public ::testing::Test
{
  public:
    void SetUp() final { memory::test::AllocatorMock::setAllocatorMock(allocatorMock_); }

    static bool checkMsgHeader(const MiddlewareMessage& lhs, const MiddlewareMessage& rhs)
    {
        const MiddlewareMessage::Header& lhsHeader = lhs.getHeader();
        const MiddlewareMessage::Header& rhsHeader = rhs.getHeader();
        return lhs.getSourceClusterId() == rhs.getSourceClusterId() &&
               lhs.getTargetClusterId() == rhs.getTargetClusterId() && lhsHeader.serviceId == rhsHeader.serviceId &&
               lhsHeader.memberId == rhsHeader.memberId && lhsHeader.serviceInstanceId == rhsHeader.serviceInstanceId &&
               lhs.getAddressId() == rhs.getAddressId() && lhsHeader.requestId == rhsHeader.requestId &&
               lhs.getFlags() == rhs.getFlags() && lhs.isSkeletonTarget() == rhs.isSkeletonTarget() &&
               lhs.isProxyTarget() == rhs.isProxyTarget() && lhs.hasError() == rhs.hasError() &&
               lhs.isEvent() == rhs.isEvent() && lhs.hasOutArgs() == rhs.hasOutArgs();
    }

  private:
    testing::NiceMock<memory::test::AllocatorMock> allocatorMock_;
};

/**
 * @brief Test the creation of a skeleton response
 *
 */
TEST_F(MiddlewareMessageTest, TestSkeletonResponseCreation)
{
    // ARRANGE
    const MiddlewareMessage::Header header{0x0FD9U, 0x8001U, 0x0010U, 0x0002U};
    const auto sourceClusterId = static_cast<ClusterId>(0xA0U);
    const auto targetClusterId = static_cast<ClusterId>(0x10U);
    const AddressId addressId = 0x02U;
    MiddlewareMessage msg = MiddlewareMessage::createResponse(header, sourceClusterId, targetClusterId, addressId);

    // ACT && ASSERT
    EXPECT_TRUE(msg.hasOutArgs());
    EXPECT_TRUE(msg.isProxyTarget());
    EXPECT_FALSE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.isEvent());
    EXPECT_FALSE(msg.hasError());
    EXPECT_EQ(msg.getErrorState(), ErrorState::NoError);
}

/**
 * @brief Test the creation of a proxy request
 *
 */
TEST_F(MiddlewareMessageTest, TestProxyRequestCreation)
{
    // ARRANGE
    const MiddlewareMessage::Header header{0x0237U, 0x0051U, 0x0610U, 0x0011U};
    const auto sourceClusterId = static_cast<ClusterId>(0xA6U);
    const auto targetClusterId = static_cast<ClusterId>(0x23U);
    const AddressId addressId = 0x05U;
    MiddlewareMessage msg = MiddlewareMessage::createRequest(header, sourceClusterId, targetClusterId, addressId);

    // ACT && ASSERT
    EXPECT_TRUE(msg.isSkeletonTarget());
    EXPECT_TRUE(msg.hasOutArgs());
    EXPECT_FALSE(msg.isProxyTarget());
    EXPECT_FALSE(msg.isEvent());
    EXPECT_FALSE(msg.hasError());
    EXPECT_EQ(msg.getErrorState(), ErrorState::NoError);
}

/**
 * @brief Test the creation of a proxy fire and forget request
 *
 */
TEST_F(MiddlewareMessageTest, TestProxyFireAndForgetRequestCreation)
{
    // ARRANGE
    const ServiceId serviceId{0x0089U};
    const MemberId memberId{0x0021U};
    const InstanceId instanceId{0x0710U};
    const auto sourceClusterId = static_cast<ClusterId>(0x06U);
    const auto targetClusterId = static_cast<ClusterId>(0x87U);
    MiddlewareMessage msg = MiddlewareMessage::createFireAndForgetRequest(
        serviceId, memberId, instanceId, sourceClusterId, targetClusterId);

    // ACT && ASSERT
    EXPECT_TRUE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.hasOutArgs());
    EXPECT_FALSE(msg.isProxyTarget());
    EXPECT_FALSE(msg.isEvent());
    EXPECT_FALSE(msg.hasError());
    EXPECT_EQ(msg.getErrorState(), ErrorState::NoError);
}

/**
 * @brief Test the creation of an error response
 *
 */
TEST_F(MiddlewareMessageTest, TestErrorResponseCreation)
{
    // ARRANGE
    const MiddlewareMessage::Header header{0x1CD7U, 0x005DU, 0x0609U, 0x0000U};
    const auto sourceClusterId = static_cast<ClusterId>(0xC3U);
    const auto targetClusterId = static_cast<ClusterId>(0xF1U);
    const AddressId addressId = 0x29U;
    ErrorState error = ErrorState::SerializationError;
    MiddlewareMessage msg =
        MiddlewareMessage::createErrorResponse(header, sourceClusterId, targetClusterId, addressId, error);

    // ACT && ASSERT
    EXPECT_TRUE(msg.hasError());
    EXPECT_TRUE(msg.isProxyTarget());
    EXPECT_FALSE(msg.hasOutArgs());
    EXPECT_FALSE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.isEvent());
    EXPECT_EQ(msg.getErrorState(), error);
}

/**
 * @brief Test default constructor, which currently needs to be empty.
 *        What we do here is use new-placement operator and then check
 *        if the values haven't been changed by the default constructor.
 */
TEST_F(MiddlewareMessageTest, TestDefaultConstructor)
{
    // ARRAGE - Create a ProxyRequest
    const MiddlewareMessage::Header header{0xABD7U, 0xA951U, 0xC910U, 0x5011U};
    const auto sourceClusterId = static_cast<ClusterId>(0xEDU);
    const auto targetClusterId = static_cast<ClusterId>(0x81U);
    const AddressId addressId = 0x36U;
    MiddlewareMessage msg = MiddlewareMessage::createRequest(header, sourceClusterId, targetClusterId, addressId);

    const MiddlewareMessage oldMsg = msg;

    // ACT
    // Default constructor will be called and shouldn't change the pre-allocated values
    new (&msg) MiddlewareMessage();

    // ASSERT
    EXPECT_TRUE(checkMsgHeader(oldMsg, msg));
}

/**
 * @brief Test the creation of an event message
 *
 */
TEST_F(MiddlewareMessageTest, TestEventCreation)
{
    // ARRANGE
    const ServiceId serviceId{0x018FU};
    const MemberId memberId{0x0241U};
    const InstanceId instanceId{0x010BU};
    const auto sourceClusterId = static_cast<ClusterId>(0x86U);
    const auto targetClusterId = static_cast<ClusterId>(0x90U);
    MiddlewareMessage msg = MiddlewareMessage::createEvent(serviceId, memberId, instanceId, sourceClusterId);
    msg.setTargetClusterId(targetClusterId);

    // ACT && ASSERT
    EXPECT_TRUE(msg.isEvent());
    EXPECT_TRUE(msg.isProxyTarget());
    EXPECT_FALSE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.hasError());
    EXPECT_FALSE(msg.hasOutArgs());
    EXPECT_EQ(msg.getErrorState(), ErrorState::NoError);
}

/**
 * @brief Test setPayload method when the payload is smaller then the message's internal buffer.
 *
 */
TEST_F(MiddlewareMessageTest, TestSetInternalPayload)
{
    // ARRANGE
    // Create a Proxy Request and create and instantiate a user type
    const MiddlewareMessage::Header header{0xDDD7U, 0x0891U, 0xCA10U, 0x002DU};
    const auto sourceClusterId = static_cast<ClusterId>(0x0DU);
    const auto targetClusterId = static_cast<ClusterId>(0x89U);
    const AddressId addressId = 0x22U;
    MiddlewareMessage msg = MiddlewareMessage::createRequest(header, sourceClusterId, targetClusterId, addressId);

    struct UserType
    {
        uint32_t a{};
        uint32_t b{};
        uint32_t c{};
        uint32_t d{};
    };
    const UserType obj{0xFF21U, 0xA351U, 0xABC1U, 0x9181U};

    // ACT
    HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);
    const UserType& storedObj = MessageAllocator::getInstance().readPayload<UserType>(msg);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::Ok);
    EXPECT_EQ(obj.a, storedObj.a);
    EXPECT_EQ(obj.b, storedObj.b);
    EXPECT_EQ(obj.c, storedObj.c);
    EXPECT_EQ(obj.d, storedObj.d);
}

/**
 * @brief Test setPayload method when the payload is bigger then the message's internal buffer.
 *
 */
TEST_F(MiddlewareMessageTest, TestSetExternalPayload)
{
    // ARRANGE
    // Create a Proxy Request and create and instantiate a user type
    const MiddlewareMessage::Header header{0x54D7U, 0x8851U, 0xC990U, 0x5051U};
    const auto sourceClusterId = static_cast<ClusterId>(0x7CU);
    const auto targetClusterId = static_cast<ClusterId>(0x1DU);
    const AddressId addressId = 0x66U;
    MiddlewareMessage msg = MiddlewareMessage::createRequest(header, sourceClusterId, targetClusterId, addressId);

    struct UserType
    {
        uint32_t a{};
        uint32_t b{};
        uint32_t c{};
        uint32_t d{};
        uint32_t e{};
        uint32_t f{};
    };
    const UserType obj{0xFF21U, 0xA351U, 0xABC1U, 0x9181U, 0xE181U, 0x97F1U};

    // ACT
    HRESULT ret = MessageAllocator::getInstance().allocate(obj, msg);

    // ASSERT
    EXPECT_EQ(ret, HRESULT::Ok);

    // ASSERT
    // Check if values are the same as original ones
    const UserType& storedObj = MessageAllocator::getInstance().readPayload<UserType>(msg);

    EXPECT_EQ(obj.a, storedObj.a);
    EXPECT_EQ(obj.b, storedObj.b);
    EXPECT_EQ(obj.c, storedObj.c);
    EXPECT_EQ(obj.d, storedObj.d);
    EXPECT_EQ(obj.e, storedObj.e);
}

}  // namespace middleware::core::test
