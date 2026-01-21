#pragma once

#include <cstdint>

namespace middleware::time
{
/**
 * @brief getter of the current SystemTime in milliseconds
 *
 * @return uint32_t
 */
extern uint32_t getCurrentTimeInMs();

/**
 * @brief getter of the current SystemTime in microseconds
 *
 * @return uint32_t
 */
extern uint32_t getCurrentTimeInUs();

}  // namespace middleware::time
