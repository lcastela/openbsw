#include <etl/limits.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "DslLogger.h"
#include "middleware/core/LoggerApi.h"

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
    logger::LogLevel const level = logger::LogLevel::Error;
    logger::Error const error    = logger::Error::Allocation;
    const HRESULT res            = HRESULT::CannotAllocatePayload;
    core::Message const msg{};

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(
        level,
        "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d s:%d",
        error,
        res,
        msg.getHeader().srcClusterId,
        msg.getHeader().tgtClusterId,
        msg.getHeader().serviceId,
        msg.getHeader().serviceInstanceId,
        msg.getHeader().memberId,
        msg.getHeader().requestId,
        static_cast<uint32_t>(sizeof(msg)));
    logger_mock_.EXPECT_EVENT_LOG(
        logger::LogLevel::Error,
        error,
        res,
        msg.getHeader().srcClusterId,
        msg.getHeader().tgtClusterId,
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
    logger::LogLevel const level     = logger::LogLevel::Critical;
    logger::Error const error        = logger::Error::ProxyInitialization;
    const HRESULT res                = HRESULT::TransceiverInitializationFailed;
    uint16_t const serviceId         = etl::numeric_limits<uint16_t>::max();
    uint16_t const serviceInstanceId = etl::numeric_limits<uint16_t>::max();
    uint8_t const sourceCluster
        = static_cast<uint8_t>(etl::numeric_limits<etl::underlying_type_t<uint8_t>>::max());

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(
        level,
        "e:%d r:%d SC:%d S:%d I:%d",
        error,
        res,
        sourceCluster,
        serviceId,
        serviceInstanceId);
    logger_mock_.EXPECT_EVENT_LOG(level, error, res, sourceCluster, serviceId, serviceInstanceId);
    middleware::logger::logInitFailure(
        level, error, res, serviceId, serviceInstanceId, sourceCluster);
}

TEST_F(LoggerApiTest, TestLogMessageSendingFailure)
{
    // ARRANGE
    logger::LogLevel const level = logger::LogLevel::Error;
    logger::Error const error    = logger::Error::DispatchMessage;
    const HRESULT res            = HRESULT::ServiceNotFound;
    core::Message const msg{};

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(
        level,
        "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d",
        error,
        res,
        msg.getHeader().srcClusterId,
        msg.getHeader().tgtClusterId,
        msg.getHeader().serviceId,
        msg.getHeader().serviceInstanceId,
        msg.getHeader().memberId,
        msg.getHeader().requestId);
    logger_mock_.EXPECT_EVENT_LOG(
        logger::LogLevel::Error,
        error,
        res,
        msg.getHeader().srcClusterId,
        msg.getHeader().tgtClusterId,
        msg.getHeader().serviceId,
        msg.getHeader().serviceInstanceId,
        msg.getHeader().memberId,
        msg.getHeader().requestId);
    middleware::logger::logMessageSendingFailure(level, error, res, msg);
}

TEST_F(LoggerApiTest, TestLogCrossThreadViolation)
{
    // ARRANGE
    logger::LogLevel const level     = logger::LogLevel::Critical;
    logger::Error const error        = logger::Error::ProxyCrossThreaViolation;
    uint16_t const serviceId         = etl::numeric_limits<uint16_t>::max();
    uint16_t const serviceInstanceId = etl::numeric_limits<uint16_t>::max();
    uint8_t const sourceCluster
        = static_cast<uint8_t>(etl::numeric_limits<etl::underlying_type_t<uint8_t>>::max());
    uint32_t const initId        = etl::numeric_limits<uint32_t>::max();
    uint32_t const currentTaskId = etl::numeric_limits<uint32_t>::max();

    // ACT && ASSERT
    logger_mock_.EXPECT_LOG(
        level,
        "e:%d SC:%d S:%d I:%d T0:%d T1:%d",
        error,
        sourceCluster,
        serviceId,
        serviceInstanceId,
        initId,
        currentTaskId);
    logger_mock_.EXPECT_EVENT_LOG(
        level, error, sourceCluster, serviceId, serviceInstanceId, initId, currentTaskId);
    middleware::logger::logCrossThreadViolation(
        level, error, sourceCluster, serviceId, serviceInstanceId, initId, currentTaskId);
}

} // namespace middleware::core::test
