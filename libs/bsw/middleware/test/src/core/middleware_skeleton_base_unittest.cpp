#include <stdint.h>

#include <etl/span.h>

#include "dsl_logger.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "middleware/core/icluster_connection.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/skeleton_base.h"
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

class Skeleton : public SkeletonBase
{
  public:
    HRESULT init(InstanceId instanceId)
    {
        return SkeletonBase::initFromInstancesDatabase(instanceId, etl::span(INSTANCESDATABASE));
    }

    HRESULT initEmptyDatabase(InstanceId instanceId)
    {
        return SkeletonBase::initFromInstancesDatabase(instanceId, etl::span(EMPTYINSTANCESDATABASE));
    }

    HRESULT initBadDatabase(InstanceId instanceId)
    {
        return SkeletonBase::initFromInstancesDatabase(instanceId, etl::span(BADINSTANCESDATABASE));
    }

    void checkCrossThreadError(const uint32_t initId) { return SkeletonBase::checkCrossThreadError(initId); }

    ServiceId getServiceId() const override { return serviceId_; }
    HRESULT onNewMessageReceived(const MiddlewareMessage& msg) override { return HRESULT::NotImplemented; }

  private:
    ServiceId serviceId_{0x10U};
};

class SkeletonBaseTest : public ::testing::Test
{
  public:
    void SetUp() override
    {
        logger_mock_.setup();

        const HRESULT res = skeleton_.init(kValidinstanceid);
        EXPECT_EQ(res, HRESULT::Ok);
        EXPECT_TRUE(skeleton_.isInitialized());
    }

    void TearDown() override { logger_mock_.teardown(); }

  protected:
    const InstanceId kValidinstanceid{1U};
    const InstanceId kInvalidinstanceid{100U};

    Skeleton skeleton_;
    middleware::logger::test::DslLogger logger_mock_{};
};

using SkeletonBaseDeathTest = SkeletonBaseTest;

/**
 * @brief Test initialization from database
 *        Test cases:
 *        - A valid init
 *        - An init with an invalidInstanceId
 *        - [MISSING] An init with an already used instanceId
 *        - Init from an empty Instances Database
 *        - Init from a bad Instances Database
 *         - Reinit skeleton
 */
TEST_F(SkeletonBaseTest, TestInitFromDatabase)
{
    // ARRANGE
    Skeleton skeleton;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    HRESULT res = skeleton.init(kValidinstanceid);
    EXPECT_EQ(res, HRESULT::Ok);
    EXPECT_TRUE(skeleton.isInitialized());

    res = skeleton.init(kValidinstanceid);
    EXPECT_EQ(res, HRESULT::Ok);
    EXPECT_TRUE(skeleton.isInitialized());
}

TEST_F(SkeletonBaseTest, TestInitFromDatabaseWithInvalidInstanceId)
{
    // ARRANGE
    Skeleton skeleton;

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Critical,
                                  logger::Error::SkeletonInitialization,
                                  HRESULT::InstanceNotFound,
                                  core::INVALID_CLUSTER_ID,
                                  skeleton.getServiceId(),
                                  kInvalidinstanceid);

    // ACT & ASSERT
    const HRESULT res = skeleton.init(kInvalidinstanceid);

    EXPECT_EQ(res, HRESULT::InstanceNotFound);
    EXPECT_FALSE(skeleton.isInitialized());
}

TEST_F(SkeletonBaseTest, TestInitWithEmptyDatabase)
{
    // ARRANGE
    Skeleton skeleton;

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Critical,
                                  logger::Error::SkeletonInitialization,
                                  HRESULT::NoClientsAvailable,
                                  core::INVALID_CLUSTER_ID,
                                  skeleton.getServiceId(),
                                  kValidinstanceid);

    // ACT & ASSERT
    const HRESULT res = skeleton.initEmptyDatabase(kValidinstanceid);

    EXPECT_EQ(res, HRESULT::NoClientsAvailable);
    EXPECT_FALSE(skeleton.isInitialized());
}

TEST_F(SkeletonBaseTest, TestInitFromWrongDatabase)
{
    // ARRANGE
    Skeleton skeleton;

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Critical,
                                  logger::Error::SkeletonInitialization,
                                  HRESULT::TransceiverInitializationFailed,
                                  core::INVALID_CLUSTER_ID,
                                  skeleton.getServiceId(),
                                  kValidinstanceid);

    // ACT & ASSERT
    const HRESULT res = skeleton.initBadDatabase(kValidinstanceid);

    EXPECT_EQ(res, HRESULT::TransceiverInitializationFailed);
    EXPECT_FALSE(skeleton.isInitialized());
}

/**
 * @brief Test sendMessage
 *        Test cases:
 *        - Valid  Target Cluster
 *        - Invalid Target Cluster
 *
 */
TEST_F(SkeletonBaseTest, TestSendMessage)
{
    // ARRANGE
    const auto tgtClusterId = static_cast<ClusterId>(2U);
    MiddlewareMessage::Header header{skeleton_.getServiceId(), 0x8001, INVALID_REQUEST_ID, skeleton_.getInstanceId()};
    MiddlewareMessage validMsg =
        MiddlewareMessage::createResponse(header, skeleton_.getSourceClusterId(), tgtClusterId, INVALID_ADDRESS_ID);

    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    const HRESULT res = skeleton_.sendMessage(validMsg);
    EXPECT_EQ(res, HRESULT::Ok);
}

TEST_F(SkeletonBaseTest, TestSendInvalidMessage)
{
    // ARRANGE
    const HRESULT expectedResult = HRESULT::ClusterIdNotFoundOrTransceiverNotRegistered;
    MiddlewareMessage::Header header{skeleton_.getServiceId(), 0x8001, INVALID_REQUEST_ID, skeleton_.getInstanceId()};
    MiddlewareMessage invalidMsg = MiddlewareMessage::createResponse(
        header, skeleton_.getSourceClusterId(), skeleton_.getSourceClusterId(), INVALID_ADDRESS_ID);

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  logger::Error::SendMessage,
                                  expectedResult,
                                  invalidMsg.getSourceClusterId(),
                                  invalidMsg.getTargetClusterId(),
                                  invalidMsg.getHeader().serviceId,
                                  invalidMsg.getHeader().serviceInstanceId,
                                  invalidMsg.getHeader().memberId,
                                  invalidMsg.getHeader().requestId);

    // ACT & ASSERT
    const HRESULT res = skeleton_.sendMessage(invalidMsg);
    EXPECT_EQ(res, expectedResult);
}

TEST_F(SkeletonBaseTest, TestSendMessageFromUnknownSkeleton)
{
    // ARRANGE
    const HRESULT expectedResult = HRESULT::ClusterIdNotFoundOrTransceiverNotRegistered;
    Skeleton skeleton;
    const ClusterId tgtClusterId = static_cast<ClusterId>(2U);
    MiddlewareMessage::Header header{skeleton_.getServiceId(), 0x8001, INVALID_REQUEST_ID, skeleton_.getInstanceId()};
    MiddlewareMessage validMsg =
        MiddlewareMessage::createResponse(header, skeleton_.getSourceClusterId(), tgtClusterId, INVALID_ADDRESS_ID);

    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  logger::Error::SendMessage,
                                  expectedResult,
                                  validMsg.getSourceClusterId(),
                                  validMsg.getTargetClusterId(),
                                  validMsg.getHeader().serviceId,
                                  validMsg.getHeader().serviceInstanceId,
                                  validMsg.getHeader().memberId,
                                  validMsg.getHeader().requestId);

    // ACT & ASSERT
    const HRESULT res = skeleton.sendMessage(validMsg);
    EXPECT_EQ(res, HRESULT::ClusterIdNotFoundOrTransceiverNotRegistered);
}

/**
 * @brief Test getSourceClusterId
 *        Test cases:
 *        - Inited skeleton
 *        - Not inited skeleton
 *
 */

TEST_F(SkeletonBaseTest, TestGetSourceClusterId)
{
    // ARRANGE

    // ACT
    const ClusterId clusterId = skeleton_.getSourceClusterId();

    // ASSERT
    EXPECT_EQ(clusterId, static_cast<ClusterId>(1U));
}

TEST_F(SkeletonBaseTest, TestGetSourceClusterIdFromNotInitSkeleton)
{
    // ARRANGE
    Skeleton skeleton;

    // ACT
    const ClusterId clusterId = skeleton.getSourceClusterId();

    // ASSERT
    EXPECT_EQ(clusterId, static_cast<ClusterId>(INVALID_CLUSTER_ID));
}

/**
 * @brief Test CheckCrossThreadError
 *        Test cases:
 *        - Process is the same
 *        - Process is NOT the same
 *
 */

TEST_F(SkeletonBaseDeathTest, TestCheckCrossThreadError)
{
    // ARRANGE
    const uint32_t goodProcess = 0U;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    skeleton_.checkCrossThreadError(goodProcess);
}

TEST_F(SkeletonBaseDeathTest, TestCheckCrossThreadErrorWithNotInitSkeleton)
{
    // ARRANGE
    Skeleton skeleton;
    const uint32_t goodProcess = 0U;
    logger_mock_.EXPECT_NO_LOGGING();

    // ACT & ASSERT
    skeleton.checkCrossThreadError(goodProcess);
}

TEST_F(SkeletonBaseDeathTest, TestCheckCrossThreadErrorAssert)
{
    // ARRANGE
    const uint32_t wrongProcess = 1234U;

    // ACT & ASSERT
    EXPECT_DEATH(
        {
            logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                          logger::Error::SkeletonCrossThreaViolation,
                                          skeleton_.getSourceClusterId(),
                                          skeleton_.getServiceId(),
                                          skeleton_.getInstanceId(),
                                          wrongProcess,
                                          0U);
            skeleton_.checkCrossThreadError(wrongProcess);
        },
        "Assertion `false' failed.");
}

/**
 * @brief Test getClusterConnections
 *        Test cases:
 *        - Inited skeleton
 *        - Not inited skeleton
 */
TEST_F(SkeletonBaseTest, TestGetClusterConnections)
{
    // ARRANGE
    // ACT & ASSERT
    EXPECT_FALSE(skeleton_.getClusterConnections().empty());
}

TEST_F(SkeletonBaseTest, TestGetClusterConnectionsFromNotInitSkeleton)
{
    // ARRANGE
    Skeleton skeleton;
    // ACT & ASSERT
    EXPECT_TRUE(skeleton.getClusterConnections().empty());
}

}  // namespace test
}  // namespace core
}  // namespace middleware
