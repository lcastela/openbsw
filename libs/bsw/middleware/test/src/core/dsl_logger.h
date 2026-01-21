#pragma once

#include <cstdarg>
#include <cstdint>
#include <string>

#include <etl/array.h>
#include <etl/span.h>
#include <etl/vector.h>
#include <gmock/gmock.h>

#include "middleware/core/LoggerApi.h"
#include "middleware/logger/Logger.h"
#include "mock/logger_mock.h"

namespace middleware::logger::test
{

using ::testing::_;
using ::testing::Cardinality;
using ::testing::ElementsAreArray;
using ::testing::Exactly;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;

class DslLogger
{
public:
    NiceMock<mock::LoggerMock> _mock;

    void setup() {}

    void teardown() {}

    template<typename... Args>
    void EXPECT_LOG(LogLevel const level, std::string const& format, Args... args)
    {
        etl::vector<uint32_t, sizeof(uint32_t) * sizeof...(Args)> vec;
        ((vec.push_back(static_cast<uint32_t>(args))),
         ...); // Fold expression to iterate and process each argument
        EXPECT_CALL(_mock, log(level, StrEq(format.c_str()), ElementsAreArray(vec)))
            .Times(Exactly(1U))
            .WillRepeatedly(Return());
    }

    template<typename... Args>
    void EXPECT_EVENT_LOG(LogLevel const level, Error const error, Args... args)
    {
        static etl::array<uint8_t, sizeof(uint32_t) + sizeof(Error) + count_bytes<Args...>::VALUE>
            buffer{};

        uint32_t const messageId = logger::getMessageId(error);

        uint32_t index = 0U;
        memcpy(&buffer[index], &messageId, sizeof(messageId));
        index += sizeof(messageId);
        memcpy(&buffer[index], &error, sizeof(error));
        index += sizeof(error);
        ((memcpy(&buffer[index], &args, sizeof(args)), index += sizeof(args)), ...);

        EXPECT_CALL(_mock, log_binary(level, ElementsAreArray(buffer)))
            .Times(Exactly(1U))
            .WillRepeatedly(Return());
    }

    void EXPECT_NO_LOG() { EXPECT_CALL(_mock, log(_, _, _)).Times(0); }

    void EXPECT_NO_BINARY_LOG() { EXPECT_CALL(_mock, log_binary(_, _)).Times(0); }

    void EXPECT_NO_LOGGING()
    {
        EXPECT_NO_LOG();
        EXPECT_NO_BINARY_LOG();
    }
};

} // namespace middleware::logger::test
