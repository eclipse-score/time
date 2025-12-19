/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYNCHRONIZEDTIMEBASECOMMON_H_
#define SCORE_TIME_SYNCHRONIZEDTIMEBASECOMMON_H_

#include <chrono>
#include <memory>
#include <optional>
#include <string_view>

#include "score/span.hpp"

#include "score/time/utility/ITimeBaseAccessor.h"

#include "score/time/synchronized_time_base_status.h"
#include "score/time/timestamp.h"

namespace score {
namespace time {

class ITimeBaseReader;

class SynchronizedTimeBaseCommon final {
   public:
    // static helpers for provider and consumer
    static uint32_t GetTimeBaseDomainId(std::string_view timebase_name);
    static std::optional<Timestamp> GetCurrentTime(ITimeBaseReader& reader);
    static std::optional<UserDataView> GetUserData(ITimeBaseReader& reader);
    static TimestampWithStatus GetTimestampFromNs(std::chrono::nanoseconds v);
    static UserDataView SanitizeUserData(const score::cpp::span<const std::byte>& ud);
    static std::optional<SynchronizationStatus> GetSynchronizationStatus(ITimeBaseReader& reader);

   private:
    SynchronizedTimeBaseCommon();
    static std::optional<Timestamp> GetInterpolatedTimestamp(const TimestampWithStatus& ts,
                                                                   VirtualLocalTime ts_vlt);
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYNCHRONIZEDTIMEBASECOMMON_H_
