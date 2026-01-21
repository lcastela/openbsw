#pragma once

#include <cstdint>

#include <etl/span.h>

namespace middleware::logger
{

/**
 * Log levels available for middleware code
 */
enum class LogLevel : uint8_t
{
    None     = 0U,
    Critical = 1U,
    Error    = 2U,
    Warning  = 3U,
    Info     = 4U,
    Debug    = 5U,
    Trace    = 6U
};

enum class Error : uint8_t
{
    Allocation                  = 0U,
    Deallocation                = 1U,
    ProxyInitialization         = 2U,
    SkeletonInitialization      = 3U,
    DispatchMessage             = 4U,
    SendMessage                 = 5U,
    ProxyCrossThreaViolation    = 6U,
    SkeletonCrossThreaViolation = 7U,
};

/**
 * Implementation of the generic logging function, whose implementation needs to be given
 * when integrating middleware in a new platform.
 */
void log(LogLevel const level, char const* const f, ...);

/**
 * Implementation of the generic logging function, whose implementation needs to be given
 * when integrating middleware in a new platform.
 */
void log_binary(LogLevel const level, etl::span<uint8_t const> const data);

/**
 * Function that returns a message id associated with the error type.
 * This is useful to extend the logs to contain possible DLT information.
 */
uint32_t getMessageId(Error const id);

} // namespace middleware::logger
