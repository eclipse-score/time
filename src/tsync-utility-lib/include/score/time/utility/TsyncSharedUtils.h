/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TSYNCSHAREDUTILS_H_
#define SCORE_TIME_UTILITY_TSYNCSHAREDUTILS_H_

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "score/span.hpp"

#include "score/time/synchronized_time_base_status.h"
#include "score/time/utility/TsyncNamedSemaphore.h"

namespace score {
namespace time {

using VirtualLocalTime = std::chrono::nanoseconds;
using UserData = std::array<std::byte, 4u>;
using UserDataView = score::cpp::span<const std::byte>;

struct TimestampWithStatus {
    SynchronizationStatus status{};
    std::chrono::nanoseconds nanoseconds{};
    std::chrono::seconds seconds{};
};

/// @brief Shared utilities class supply shared utilities functions for deamon, lib and ptp-lib
class TsyncSharedUtils final {
public:
    /**
     * @brief Create a new named semaphore object from an id
     *
     * @details
     * The semaphore instance is owned by the caller.
     * Using the domain id for the semaphore name. ptp-lib has no concept of domain names.
     * Transmission is handled by calling TsyncNamedSemaphore::unlock()
     *
     * @param time_domain_id id of the time domain
     * @param as_owner owner flag for named semaphore file, if true the semaphore will be created
     * @return A TsyncNamedSamphore object
     */
    static TsyncNamedSemaphore CreateTransmissionSemaphore(std::uint32_t time_domain_id, bool as_owner = false);

    /**
     * @brief Get current virtual local time value
     *
     * @return std::optional<std::chrono::nanoseconds>    std::nullopt: Failed
     *                         a pointer current virtual local time in nanoseconds: Success
     */
    static std::optional<VirtualLocalTime> GetCurrentVirtualLocalTime();

    /**
     * @brief Get provider transmission semaphore name
     *
     * @param time_domain_id id of time domain
     * @return the name of the semaphore file corresponding to time_domain_id
     */
    static std::string GetTransmissionSemaphoreName(std::uint32_t time_domain_id);
};

// checks numeric limits and a given upper bound
template <typename OUT_TYPE, typename ADDER_TYPE>
constexpr inline
    typename std::enable_if<std::is_integral<OUT_TYPE>::value && std::is_integral<ADDER_TYPE>::value, bool>::type
    IsInBounds(OUT_TYPE a, ADDER_TYPE b, OUT_TYPE max) {
    constexpr OUT_TYPE type_max = std::numeric_limits<OUT_TYPE>::max();
    // make sure a+b does not wrap
    return ((max < type_max) && (max > b) && (max - b > a));
}

// only checks numeric limits
template <typename OUT_TYPE, typename ADDER_TYPE>
constexpr inline
    typename std::enable_if<std::is_integral<OUT_TYPE>::value && std::is_integral<ADDER_TYPE>::value, bool>::type
    IsInBounds(OUT_TYPE a, ADDER_TYPE b) {
    constexpr OUT_TYPE type_max = std::numeric_limits<OUT_TYPE>::max();
    // make sure a+b does not wrap
    return (type_max - b > a);
}

template <typename RESULT_TYPE, typename ADDER_TYPE>
constexpr inline
    typename std::enable_if<std::is_integral<RESULT_TYPE>::value && std::is_integral<ADDER_TYPE>::value, bool>::type
    SafeAdd(RESULT_TYPE& a, ADDER_TYPE b, RESULT_TYPE max) {
    if (IsInBounds(a, b, max)) {
        a += static_cast<RESULT_TYPE>(b);
        return true;
    }

    return false;
}

// only checks numeric limits
template <typename RESULT_TYPE, typename ADDER_TYPE>
constexpr inline
    typename std::enable_if<std::is_integral<RESULT_TYPE>::value && std::is_integral<ADDER_TYPE>::value, bool>::type
    SafeAdd(RESULT_TYPE& a, ADDER_TYPE b) {
    if (IsInBounds(a, b)) {
        a += static_cast<RESULT_TYPE>(b);
        return true;
    }

    return false;
}

template <typename RESULT_TYPE, typename SUBTRACTOR_TYPE>
constexpr inline typename std::enable_if<std::is_unsigned<RESULT_TYPE>::value && std::is_integral<RESULT_TYPE>::value &&
                                             std::is_integral<SUBTRACTOR_TYPE>::value,
                                         bool>::type
SafeSub(RESULT_TYPE& a, SUBTRACTOR_TYPE b) {
    if (b > a) {
        return false;
    }

    a -= b;
    return true;
}

template <class T, class... Args>
constexpr auto make_array(Args&&... args) {
    return std::array{static_cast<T>(std::forward<Args>(args))...};
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TSYNCSHAREDUTILS_H_
