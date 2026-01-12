/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SynchronizedTimeBaseCommon.h"

#include <mutex>

#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncSharedUtils.h"

using namespace std::chrono;

namespace score {
namespace time {

// clang-format off
// RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "This object will not be accessed during global static initialization.", true)
TsyncIdMappingsHandler mappings_handler{};
// clang-format on

uint32_t SynchronizedTimeBaseCommon::GetTimeBaseDomainId(std::string_view timebase_name) {
    auto domain_name = timebase_name;

    if (domain_name.front() == '/') {
        domain_name.remove_prefix(1u);
    }

    auto id = mappings_handler.GetDomainId(domain_name);
    return *id;
}

std::optional<Timestamp> SynchronizedTimeBaseCommon::GetCurrentTime(ITimeBaseReader& reader) {
    reader.GetAccessor().Open();
    TimestampWithStatus ts;
    VirtualLocalTime lt;
    {
        std::lock_guard<ITimeBaseReader> lock(reader);
        if (reader.Read(ts) && reader.Read(lt)) {
            return GetInterpolatedTimestamp(ts, lt);
        }
    }

    return std::nullopt;
}

std::optional<Timestamp> SynchronizedTimeBaseCommon::GetInterpolatedTimestamp(const TimestampWithStatus& ts,
                                                                                    VirtualLocalTime ts_vlt) {
    auto vlt{TsyncSharedUtils::GetCurrentVirtualLocalTime()};
    if (!vlt) {
        return std::nullopt;
    }

    auto ts_ns = ts.nanoseconds + duration_cast<std::chrono::nanoseconds>(ts.seconds);
    VirtualLocalTime ns = ts_ns + std::chrono::nanoseconds{(*vlt).count() - ts_vlt.count()};
    auto d = duration_cast<Timestamp::duration>(ns);
    return Timestamp(d);
}

TimestampWithStatus SynchronizedTimeBaseCommon::GetTimestampFromNs(std::chrono::nanoseconds v) {
    TimestampWithStatus ts;
    auto s = duration_cast<std::chrono::seconds>(v);
    // coverity[autosar_cpp14_a4_7_1_violation] Because the implementation is designed not to overflow
    std::chrono::nanoseconds ns_remainder{v.count() - duration_cast<std::chrono::nanoseconds>(s).count()};
    ts.status = SynchronizationStatus::kNotSynchronizedUntilStartup;
    ts.seconds = s;
    ts.nanoseconds = ns_remainder;

    return ts;
}

std::optional<UserDataView> SynchronizedTimeBaseCommon::GetUserData(ITimeBaseReader& reader) {
    reader.GetAccessor().Open();
    std::lock_guard<ITimeBaseReader> lock(reader);
    UserDataView ud;

    if (AlignedSkip<ITimeBaseReader, TimestampWithStatus>(reader) &&
        AlignedSkip<ITimeBaseReader, VirtualLocalTime>(reader)) {
        if (reader.Read(ud)) {
            return ud;
        }
    }

    return std::nullopt;
}

UserDataView SynchronizedTimeBaseCommon::SanitizeUserData(const score::cpp::span<const std::byte>& ud) {
    constexpr std::size_t kMaxUserDataSize = 3u;
    if (ud.size() > kMaxUserDataSize) {
        return (ud.first(kMaxUserDataSize));
    }

    return ud;
}

std::optional<SynchronizationStatus> SynchronizedTimeBaseCommon::GetSynchronizationStatus(
    ITimeBaseReader& reader) {
    reader.GetAccessor().Open();
    SynchronizationStatus status{};
    {
        std::lock_guard<ITimeBaseReader> lock(reader);
        if (reader.Read(status)) {
            return status;
        }
    }

    return std::nullopt;
}

}  // namespace time
}  // namespace score
