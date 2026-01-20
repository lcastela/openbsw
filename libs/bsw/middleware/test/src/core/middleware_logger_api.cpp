#include <etl/limits.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dsl_logger.h"
#include "middleware/core/logger_api.h"

namespace middleware::core::test
{

class LoggerApiTest : public ::testing::Test
{
  public:
    void SetUp() override { logger_mock_.setup(); }

    void TearDown() override { logger_mock_.teardown(); }

  protected:
    middleware::logger::test::DslLogger logger_mock_{};
};

TEST_F(LoggerApiTest, TestLogAllocationFailure)
{
    // ARRANGE
    const logger::LogLevel level = logger::LogLevel::Error;
    const logger::Error error = logger::Error::Allocation;
    const HRESULT res = HRESULT::CannotAllocatePayload;
    const core::MiddlewareMessage msg{};

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(level,
                            "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d s:%d",
                            error,
                            res,
                            msg.getSourceClusterId(),
                            msg.getTargetClusterId(),
                            msg.getHeader().serviceId,
                            msg.getHeader().serviceInstanceId,
                            msg.getHeader().memberId,
                            msg.getHeader().requestId,
                            static_cast<uint32_t>(sizeof(msg)));
    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  error,
                                  res,
                                  msg.getSourceClusterId(),
                                  msg.getTargetClusterId(),
                                  msg.getHeader().serviceId,
                                  msg.getHeader().serviceInstanceId,
                                  msg.getHeader().memberId,
                                  msg.getHeader().requestId,
                                  static_cast<uint32_t>(sizeof(msg)));
    middleware::logger::logAllocationFailure(level, error, res, msg, sizeof(msg));
}

TEST_F(LoggerApiTest, TestLogInitFailure)
{
    // ARRANGE
    const logger::LogLevel level = logger::LogLevel::Critical;
    const logger::Error error = logger::Error::ProxyInitialization;
    const HRESULT res = HRESULT::TransceiverInitializationFailed;
    const uint16_t serviceId = etl::numeric_limits<uint16_t>::max();
    const core::InstanceId serviceInstanceId = etl::numeric_limits<core::InstanceId>::max();
    const core::ClusterId sourceCluster =
        static_cast<ClusterId>(etl::numeric_limits<etl::underlying_type_t<ClusterId>>::max());

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(
        level, "e:%d r:%d SC:%d S:%d I:%d", error, res, sourceCluster, serviceId, serviceInstanceId);
    logger_mock_.EXPECT_EVENT_LOG(level, error, res, sourceCluster, serviceId, serviceInstanceId);
    middleware::logger::logInitFailure(level, error, res, serviceId, serviceInstanceId, sourceCluster);
}

TEST_F(LoggerApiTest, TestLogMessageSendingFailure)
{
    // ARRANGE
    const logger::LogLevel level = logger::LogLevel::Error;
    const logger::Error error = logger::Error::DispatchMessage;
    const HRESULT res = HRESULT::ServiceNotFound;
    const core::MiddlewareMessage msg{};

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(level,
                            "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d",
                            error,
                            res,
                            msg.getSourceClusterId(),
                            msg.getTargetClusterId(),
                            msg.getHeader().serviceId,
                            msg.getHeader().serviceInstanceId,
                            msg.getHeader().memberId,
                            msg.getHeader().requestId);
    logger_mock_.EXPECT_EVENT_LOG(logger::LogLevel::Error,
                                  error,
                                  res,
                                  msg.getSourceClusterId(),
                                  msg.getTargetClusterId(),
                                  msg.getHeader().serviceId,
                                  msg.getHeader().serviceInstanceId,
                                  msg.getHeader().memberId,
                                  msg.getHeader().requestId);
    middleware::logger::logMessageSendingFailure(level, error, res, msg);
}

TEST_F(LoggerApiTest, TestLogCrossThreadViolation)
{
    // ARRANGE
    const logger::LogLevel level = logger::LogLevel::Critical;
    const logger::Error error = logger::Error::ProxyCrossThreaViolation;
    const uint16_t serviceId = etl::numeric_limits<uint16_t>::max();
    const core::InstanceId serviceInstanceId = etl::numeric_limits<core::InstanceId>::max();
    const core::ClusterId sourceCluster =
        static_cast<ClusterId>(etl::numeric_limits<etl::underlying_type_t<ClusterId>>::max());
    const uint32_t initId = etl::numeric_limits<uint32_t>::max();
    const uint32_t currentTaskId = etl::numeric_limits<uint32_t>::max();

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(level,
                            "e:%d SC:%d S:%d I:%d T0:%d T1:%d",
                            error,
                            sourceCluster,
                            serviceId,
                            serviceInstanceId,
                            initId,
                            currentTaskId);
    logger_mock_.EXPECT_EVENT_LOG(level, error, sourceCluster, serviceId, serviceInstanceId, initId, currentTaskId);
    middleware::logger::logCrossThreadViolation(
        level, error, sourceCluster, serviceId, serviceInstanceId, initId, currentTaskId);
}

}  // namespace middleware::core::test
