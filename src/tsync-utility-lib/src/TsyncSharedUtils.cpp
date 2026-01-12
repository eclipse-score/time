/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/utility/TsyncSharedUtils.h"

#include <string>

#include "score/time/utility/SysCalls.h"

using score::time::TsyncNamedSemaphore;

namespace score {
namespace time {

std::string TsyncSharedUtils::GetTransmissionSemaphoreName(std::uint32_t time_domain_id) {
    const std::string sem_name = {"time_domain_"};

    return sem_name + std::to_string(static_cast<std::int32_t>(time_domain_id));
}

TsyncNamedSemaphore TsyncSharedUtils::CreateTransmissionSemaphore(std::uint32_t time_domain_id, bool as_owner) {
    return TsyncNamedSemaphore(GetTransmissionSemaphoreName(time_domain_id), TsyncNamedSemaphore::OpenMode::Unsignaled,
                               as_owner);
}

std::optional<VirtualLocalTime> TsyncSharedUtils::GetCurrentVirtualLocalTime() {
    timespec ts;
    if (OsClockGetTime(CLOCK_MONOTONIC, &ts) == -1) {
        return std::nullopt;
    }

    std::chrono::seconds sec(ts.tv_sec);
    std::chrono::nanoseconds nsec(ts.tv_nsec);
    std::chrono::nanoseconds sec_in_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(sec);

    return (sec_in_nsec + nsec);
}

}  // namespace time
}  // namespace score
