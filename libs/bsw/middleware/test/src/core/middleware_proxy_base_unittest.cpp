#include <stdint.h>
#include <stdlib.h>

#include <etl/span.h>

#include "dsl_logger.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "middleware/core/icluster_connection.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/types.h"
#include "middleware_instances_database.h"

using testing::_;
using testing::Exactly;
using testing::NiceMock;

namespace middleware
{
namespace core
{
namespace test
{

class Proxy : public ProxyBase
{
  public:
    HRESULT init(InstanceId instanceId, ClusterId clusterId)
    {
        return ProxyBase::initFromInstancesDatabase(instanceId, clusterId, etl::span(INSTANCESDATABASE));
    }

    ClusterId getProxySourceClusterId() { return ProxyBase::getSourceClusterId(); }
    void checkCrossThreadError(const uint32_t initId) { return ProxyBase::checkCrossThreadError(initId); }

    ServiceId getServiceId() const override { return serviceId_; }
    HRESULT onNewMessageReceived(const MiddlewareMessage& msg) override { return HRESULT::NotImplemented; }

  private:
    ServiceId serviceId_{0x10U};
};

class ProxyBaseTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        logger_mock_.setup();
        const HRESULT res = proxy_.init(kValidinstanceid, kValidclustid);
        EXPECT_EQ(res, HRESULT::Ok);
        EXPECT_TRUE(proxy_.isInitialized());
    }

    void TearDown() override { logger_mock_.teardown(); }

  protected:
    const InstanceId kValidinstanceid{1U};
    const ClusterId kValidclustid{static_cast<ClusterId>(1U)};
    const InstanceId kInvalidinstanceid{100U};
    const ClusterId kInvalidclustid{static_cast<ClusterId>(100U)};

    Proxy proxy_;
    middleware::logger::test::DslLogger logger_mock_{};
};

using ProxyBaseDeathTest = ProxyBaseTest;

TEST_F(ProxyBaseTest, TestInitFromDatabase)
{
    // ARRANGE
    Proxy proxy;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    HRESULT res = proxy.init(kValidinstanceid, kValidclustid);
    EXPECT_EQ(res, HRESULT::Ok);

    EXPECT_TRUE(proxy.isInitialized());

    // re-init
    res = proxy.init(kValidinstanceid, kValidclustid);
    EXPECT_EQ(res, HRESULT::Ok);
    EXPECT_TRUE(proxy.isInitialized());
}

TEST_F(ProxyBaseTest, TestInitFromDatabaseWithInvalidInstanceId)
{
    // ARRANGE
    Proxy proxy;

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Critical,
                                  logger::Error::ProxyInitialization,
                                  HRESULT::TransceiverInitializationFailed,
                                  kValidclustid,
                                  proxy.getServiceId(),
                                  kInvalidinstanceid);

    // ACT & ASSERT
    const HRESULT res = proxy.init(kInvalidinstanceid, kValidclustid);

    EXPECT_EQ(res, HRESULT::TransceiverInitializationFailed);
    EXPECT_FALSE(proxy.isInitialized());
}

TEST_F(ProxyBaseTest, TestInitFromDatabaseWithInvalidClusterId)
{
    // ARRANGE
    Proxy proxy;

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Critical,
                                  logger::Error::ProxyInitialization,
                                  HRESULT::TransceiverInitializationFailed,
                                  kInvalidclustid,
                                  proxy.getServiceId(),
                                  kValidinstanceid);

    // ACT & ASSERT
    const HRESULT res = proxy.init(kValidinstanceid, kInvalidclustid);

    EXPECT_EQ(res, HRESULT::TransceiverInitializationFailed);
    EXPECT_FALSE(proxy.isInitialized());
}

TEST_F(ProxyBaseTest, TestGenerateMessageHeaderWithRequestId)
{
    // ARRANGE
    MiddlewareMessage msg;
    const MemberId memberId{0x15U};
    const RequestId requestId{0x05U};

    // ACT
    msg = proxy_.generateMessageHeader(memberId, requestId);

    // ASSERT
    EXPECT_EQ(msg.getSourceClusterId(), kValidclustid);
    EXPECT_EQ(msg.getTargetClusterId(), static_cast<ClusterId>(2U));  // Hardcoded for now
    EXPECT_EQ(msg.getHeader().serviceId, proxy_.getServiceId());
    EXPECT_EQ(msg.getHeader().memberId, memberId);
    EXPECT_EQ(msg.getHeader().serviceInstanceId, proxy_.getInstanceId());
    EXPECT_EQ(msg.getAddressId(), proxy_.getAddressId());
    EXPECT_EQ(msg.getHeader().requestId, requestId);
    EXPECT_TRUE(msg.hasOutArgs());
    EXPECT_TRUE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.isEvent());
}

TEST_F(ProxyBaseTest, TestGenerateMessageHeaderWithInvalidRequestId)
{
    // ARRANGE
    MiddlewareMessage msg;
    const MemberId memberId{0x15U};

    // ACT
    msg = proxy_.generateMessageHeader(memberId, INVALID_REQUEST_ID);

    // ASSERT
    EXPECT_EQ(msg.getSourceClusterId(), kValidclustid);
    EXPECT_EQ(msg.getTargetClusterId(), static_cast<ClusterId>(2U));  // Hardcoded for now
    EXPECT_EQ(msg.getHeader().serviceId, proxy_.getServiceId());
    EXPECT_EQ(msg.getHeader().memberId, memberId);
    EXPECT_EQ(msg.getHeader().serviceInstanceId, proxy_.getInstanceId());
    EXPECT_EQ(msg.getAddressId(), proxy_.getAddressId());
    EXPECT_EQ(msg.getHeader().requestId, INVALID_REQUEST_ID);
    EXPECT_FALSE(msg.hasOutArgs());
    EXPECT_TRUE(msg.isSkeletonTarget());
    EXPECT_FALSE(msg.isEvent());
}

/**
 * @brief Test generation and message sending
 *        Test cases:
 *        - Successful message sent
 *        - Not initialized
 *        - [MISSING] Too big of a payload
 *        - [MISSING] Queue Full
 *
 */

TEST_F(ProxyBaseTest, TestGenerateAndSendMessage)
{
    // ARRANGE
    etl::array<uint8_t, 2U> payloadArray{{0x00U, 0x01U}};
    const MemberId memberId{0x15U};
    const RequestId requestId{0x05U};

    // ACT
    MiddlewareMessage msg = proxy_.generateMessageHeader(memberId, requestId);

    // ASSERT
    EXPECT_EQ(MessageAllocator::getInstance().allocate(payloadArray, msg), HRESULT::Ok);
    EXPECT_EQ(proxy_.sendMessage(msg), HRESULT::Ok);
}

TEST_F(ProxyBaseTest, TestSendMessageWithNotInitProxy)
{
    // ARRANGE
    Proxy proxy;
    MiddlewareMessage msg{};

    // ASSERT
    EXPECT_EQ(proxy.sendMessage(msg), HRESULT::NotRegistered);
}

TEST_F(ProxyBaseDeathTest, TestCheckCrossThreadErrorWithSameTaskId)
{
    // ARRANGE
    const uint32_t goodProcess = 0U;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    proxy_.checkCrossThreadError(goodProcess);
}

TEST_F(ProxyBaseDeathTest, TestCheckCrossThreadErrorWithNotInitProxy)
{
    // ARRANGE
    Proxy proxy;
    const uint32_t goodProcess = 0U;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    proxy.checkCrossThreadError(goodProcess);
}

TEST_F(ProxyBaseDeathTest, TestCheckCrossThreadErrorAssert)
{
    // ARRANGE
    const uint32_t wrongProcess = 1234U;

    // ACT & ASSERT
    EXPECT_DEATH(
        {
            logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                          logger::Error::ProxyCrossThreaViolation,
                                          proxy_.getProxySourceClusterId(),
                                          proxy_.getServiceId(),
                                          proxy_.getInstanceId(),
                                          wrongProcess,
                                          0U);
            proxy_.checkCrossThreadError(wrongProcess);
        },
        "Assertion `false' failed.");
}

}  // namespace test
}  // namespace core
}  // namespace middleware
