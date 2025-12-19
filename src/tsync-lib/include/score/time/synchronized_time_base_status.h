/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYNCHRONIZED_TIME_BASE_STATUS_H_
#define SCORE_TIME_SYNCHRONIZED_TIME_BASE_STATUS_H_

#include <cstdint>

#include "score/span.hpp"
#include "score/time/timestamp.h"

namespace score {
namespace time {

/**
 * @traceid{SWS_TS_01050}
 */
enum class SynchronizationStatus : std::uint32_t {
    kNotSynchronizedUntilStartup = 0,
    kTimeOut = 1,
    kSynchronized = 2,
    kSynchToGateway = 3,
};

/**
 * @traceid{SWS_TS_01051}
 */
enum class LeapJump : std::uint32_t {
    kTimeLeapNone = 0,
    kTimeLeapFuture = 1,
    kTimeLeapPast = 2,
};

/**
 * @traceid{SWS_TS_01052}
 */
class SynchronizedTimeBaseStatus final {
public:
    /**
     * @returns SynchronizationStatus
     * @traceid{SWS_TS_01053}
     */
    SynchronizationStatus GetSynchronizationStatus() const noexcept;

    /**
     * @returns LeapJump
     * @traceid{SWS_TS_01054}
     */
    LeapJump GetLeapJump() const noexcept;

    /**
     * @returns The point in time at which this object was created. Time point is expressed in context of the clock that
     * created this object.
     * @traceid{SWS_TS_01055}
     */
    score::time::Timestamp GetCreationTime() const noexcept;

    /**
     * @returns A vector of bytes holding the user data that was set during the creation of the status.
     * A size of zero indicates that no user data is available.
     * @traceid{SWS_TS_01056}
     */
    score::cpp::span<const std::byte> GetUserData() const noexcept;

    /**
     * @traceid{SWS_TS_01057}
     */
    SynchronizedTimeBaseStatus(SynchronizedTimeBaseStatus&&) noexcept = default;
    /**
     * @traceid{SWS_TS_01058}
     */
    SynchronizedTimeBaseStatus(const SynchronizedTimeBaseStatus&) noexcept = default;
    /**
     * @traceid{SWS_TS_01059}
     */
    SynchronizedTimeBaseStatus& operator=(SynchronizedTimeBaseStatus&&) noexcept = default;
    /**
     * @traceid{SWS_TS_01060}
     */
    SynchronizedTimeBaseStatus& operator=(const SynchronizedTimeBaseStatus&) noexcept = default;
    /**
     * @brief Destructor
     */
    ~SynchronizedTimeBaseStatus() = default;

private:
    friend class TimeBaseStatusAccessMediator;
    SynchronizedTimeBaseStatus() noexcept;

    SynchronizationStatus sync_status_{};
    LeapJump leap_jump_{};
    Timestamp creation_time_{};
    score::cpp::span<const std::byte> user_data_{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYNCHRONIZED_TIME_BASE_STATUS_H_
