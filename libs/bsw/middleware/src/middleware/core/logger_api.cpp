#include "middleware/core/logger_api.h"

#include <cstddef>
#include <cstdint>

#include <etl/array.h>
#include <etl/byte_stream.h>

#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"

namespace middleware::logger
{

namespace
{

void serialize(etl::byte_stream_writer& writer, const uint8_t value)
{
    writer.write_unchecked(value);
}

void serialize(etl::byte_stream_writer& writer, const uint16_t value)
{
    writer.write_unchecked(value);
}

void serialize(etl::byte_stream_writer& writer, const uint32_t value)
{
    writer.write_unchecked(value);
}

void serialize(etl::byte_stream_writer& writer, const core::MiddlewareMessage& value)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
    static_assert(count_bytes<core::MiddlewareMessage>::VALUE == 10U,
                  "MiddlewareMessage log size in bytes exceeds the payload");

    writer.write_unchecked(static_cast<uint8_t>(value.getSourceClusterId()));
    writer.write_unchecked(static_cast<uint8_t>(value.getTargetClusterId()));
    writer.write_unchecked(static_cast<uint16_t>(value.getHeader().serviceId));
    writer.write_unchecked(static_cast<uint16_t>(value.getHeader().serviceInstanceId));
    writer.write_unchecked(static_cast<uint16_t>(value.getHeader().memberId));
    writer.write_unchecked(static_cast<uint16_t>(value.getHeader().requestId));
}

template <typename Value, typename... Values>
void serialize(etl::byte_stream_writer& writer, Value value, Values... values)
{
    serialize(writer, value);
    serialize(writer, values...);
}

template <uint32_t MAX_SIZE, typename... Values>
void serialize(etl::byte_stream_writer& writer, Values... values)
{
    static_assert(count_bytes<Values...>::VALUE == MAX_SIZE, "Total size in bytes exceeds the payload");

    serialize(writer, values...);
}
}  // namespace

void logAllocationFailure(const LogLevel level,
                          const Error error,
                          const core::HRESULT res,
                          const core::MiddlewareMessage& msg,
                          const uint32_t size)
{
    static const char* const kformat = "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d s:%d";

    etl::array<uint8_t, ALLOCATION_FAILURE_LOG_SIZE> temp{};
    etl::byte_stream_writer writer{temp, etl::endian::native};
    serialize<ALLOCATION_FAILURE_LOG_SIZE>(
        writer, getMessageId(error), static_cast<uint8_t>(error), static_cast<uint8_t>(res), msg, size);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    middleware::logger::log(level,
                            kformat,
                            error,
                            res,
                            msg.getSourceClusterId(),
                            msg.getTargetClusterId(),
                            msg.getHeader().serviceId,
                            msg.getHeader().serviceInstanceId,
                            msg.getHeader().memberId,
                            msg.getHeader().requestId,
                            size);
    middleware::logger::log_binary(level, temp);
}

void logInitFailure(const LogLevel level,
                    const Error error,
                    const core::HRESULT res,
                    const uint16_t serviceId,
                    const core::InstanceId serviceInstanceId,
                    const core::ClusterId sourceCluster)
{
    static const char* const kformat = "e:%d r:%d SC:%d S:%d I:%d";

    etl::array<uint8_t, INIT_FAILURE_LOG_SIZE> temp{};  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    etl::byte_stream_writer writer{temp, etl::endian::native};
    serialize<INIT_FAILURE_LOG_SIZE>(writer,
                                     getMessageId(error),
                                     static_cast<uint8_t>(error),
                                     static_cast<uint8_t>(res),
                                     static_cast<uint8_t>(sourceCluster),
                                     static_cast<uint16_t>(serviceId),
                                     static_cast<uint16_t>(serviceInstanceId));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    middleware::logger::log(level, kformat, error, res, sourceCluster, serviceId, serviceInstanceId);
    middleware::logger::log_binary(level, temp);
}

void logMessageSendingFailure(const LogLevel level,
                              const Error error,
                              const core::HRESULT res,
                              const core::MiddlewareMessage& msg)
{
    static const char* const kformat = "e:%d r:%d SC:%d TC:%d S:%d I:%d M:%d R:%d";

    etl::array<uint8_t, MSG_SEND_FAILURE_LOG_SIZE> temp{};  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    etl::byte_stream_writer writer{temp, etl::endian::native};
    serialize<MSG_SEND_FAILURE_LOG_SIZE>(
        writer, getMessageId(error), static_cast<uint8_t>(error), static_cast<uint8_t>(res), msg);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    middleware::logger::log(level,
                            kformat,
                            error,
                            res,
                            msg.getSourceClusterId(),
                            msg.getTargetClusterId(),
                            msg.getHeader().serviceId,
                            msg.getHeader().serviceInstanceId,
                            msg.getHeader().memberId,
                            msg.getHeader().requestId);
    middleware::logger::log_binary(level, temp);
}

void logCrossThreadViolation(const LogLevel level,
                             const Error error,
                             const core::ClusterId sourceCluster,
                             const uint16_t serviceId,
                             const core::InstanceId serviceInstanceId,
                             const uint32_t initId,
                             const uint32_t currentTaskId)
{
    static const char* const kformat = "e:%d SC:%d S:%d I:%d T0:%d T1:%d";

    etl::array<uint8_t, CROSS_THREAD_VIOLATION_LOG_SIZE> temp{};  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
    etl::byte_stream_writer writer{temp, etl::endian::native};
    serialize<CROSS_THREAD_VIOLATION_LOG_SIZE>(writer,
                                               getMessageId(error),
                                               static_cast<uint8_t>(error),
                                               static_cast<uint8_t>(sourceCluster),
                                               static_cast<uint16_t>(serviceId),
                                               static_cast<uint16_t>(serviceInstanceId),
                                               static_cast<uint32_t>(initId),
                                               static_cast<uint32_t>(currentTaskId));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    middleware::logger::log(level, kformat, error, sourceCluster, serviceId, serviceInstanceId, initId, currentTaskId);
    middleware::logger::log_binary(level, temp);
}

}  // namespace middleware::logger
