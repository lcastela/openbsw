#pragma once

#include <cstdint>

#include <etl/platform.h>

#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"

namespace middleware::logger
{

ETL_INLINE_VAR constexpr uint8_t ALLOCATION_FAILURE_LOG_SIZE = 20U;
ETL_INLINE_VAR constexpr uint8_t INIT_FAILURE_LOG_SIZE = 11U;
ETL_INLINE_VAR constexpr uint8_t MSG_SEND_FAILURE_LOG_SIZE = 16U;
ETL_INLINE_VAR constexpr uint8_t CROSS_THREAD_VIOLATION_LOG_SIZE = 18U;

template <typename... Args>
struct count_bytes;

template <typename T>
struct count_bytes<T>
{
    constexpr static size_t VALUE = sizeof(T);
};

template <>
struct count_bytes<core::MiddlewareMessage>
{
    constexpr static size_t VALUE = 10U;  // only for relevant header information
};

template <typename T, typename... Args>
struct count_bytes<T, Args...>
{
    constexpr static size_t VALUE = count_bytes<T>::VALUE + count_bytes<Args...>::VALUE;
};

/**
 * @brief Logs an allocation failure event.
 *
 * @param level The severity level of the log message (e.g., INFO, WARN, ERROR).
 * @param error The specific error that occurred during allocation.
 * @param res The HRESULT indicating the result of the allocation attempt.
 * @param msg A message object containing details about the middleware communication.
 * @param size The size of the allocation that failed.
 */
void logAllocationFailure(const LogLevel level,
                          const Error error,
                          const core::HRESULT res,
                          const core::MiddlewareMessage& msg,
                          const uint32_t size);

/**
 * @brief Logs an initialization failure event for a service.
 *
 * @param level The severity level of the log message.
 * @param error The specific error encountered during the initialization process.
 * @param res The HRESULT indicating the result of the initialization attempt.
 * @param serviceId The identifier of the service that failed to initialize.
 * @param serviceInstanceId The instance identifier of the service that failed.
 * @param sourceCluster The cluster from which the initialization failure originated.
 */
void logInitFailure(const LogLevel level,
                    const Error error,
                    const core::HRESULT res,
                    const uint16_t serviceId,
                    const core::InstanceId serviceInstanceId,
                    const core::ClusterId sourceCluster);

/**
 * @brief Logs a failure encountered while sending a message.
 *
 * @param level The severity level of the log message.
 * @param error The specific error that occurred during message sending.
 * @param res The HRESULT indicating the result of the sending attempt.
 * @param msg A message object containing details about the middleware message that failed to send.
 */
void logMessageSendingFailure(const LogLevel level,
                              const Error error,
                              const core::HRESULT res,
                              const core::MiddlewareMessage& msg);

/**
 * @brief Logs a violation of cross-thread operations.
 *
 * @param level The severity level of the log message.
 * @param error The specific error indicating the type of violation.
 * @param sourceCluster The cluster from which the violation originated.
 * @param serviceId The identifier of the service involved in the violation.
 * @param serviceInstanceId The instance identifier of the service involved.
 * @param initId The ID of the initialization operation associated with the violation.
 * @param currentTaskId The ID of the current task that caused the violation.
 */
void logCrossThreadViolation(const LogLevel level,
                             const Error error,
                             const core::ClusterId sourceCluster,
                             const uint16_t serviceId,
                             const core::InstanceId serviceInstanceId,
                             const uint32_t initId,
                             const uint32_t currentTaskId);

}  // namespace middleware::logger
