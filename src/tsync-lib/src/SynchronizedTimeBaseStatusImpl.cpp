/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/synchronized_time_base_status.h"

namespace score {
namespace time {

SynchronizedTimeBaseStatus::SynchronizedTimeBaseStatus() noexcept
    : sync_status_(SynchronizationStatus::kNotSynchronizedUntilStartup), leap_jump_(LeapJump::kTimeLeapNone) {
}

SynchronizationStatus SynchronizedTimeBaseStatus::GetSynchronizationStatus() const noexcept {
    return sync_status_;
}

LeapJump SynchronizedTimeBaseStatus::GetLeapJump() const noexcept {
    return leap_jump_;
}

score::time::Timestamp SynchronizedTimeBaseStatus::GetCreationTime() const noexcept {
    return creation_time_;
}

score::cpp::span<const std::byte> SynchronizedTimeBaseStatus::GetUserData() const noexcept {
    return user_data_;
}

}  // namespace time
}  // namespace score
