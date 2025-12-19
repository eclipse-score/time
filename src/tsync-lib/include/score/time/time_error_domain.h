/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_ERROR_DOMAIN_H
#define SCORE_TIME_ERROR_DOMAIN_H

#include "score/result/error_code.h"
#include "score/result/error_domain.h"

namespace score {
namespace time {

enum class TimeErrorCode : score::result::ErrorCode {
    kDaemonConnectionLost = 1,
    kTimeCannotSet = 2,
    kLimitsExceeded = 3
};

class TimeErrorDomain final : public score::result::ErrorDomain {
public:
    /// @traceid{SWS_CORE_00231}
    using Errc = TimeErrorCode;

    constexpr TimeErrorDomain() noexcept = default;

    /// @param errorCode  the error code value
    /// @returns the text message
    std::string_view MessageFor(const score::result::ErrorCode& errorCode) const noexcept override {
        Errc const code = static_cast<Errc>(errorCode);
        switch (code) {
            case Errc::kDaemonConnectionLost:
                return "Daemon connection lost";
            case Errc::kTimeCannotSet:
                return "Time cannot be set";
            case Errc::kLimitsExceeded:
                return "Value out of bounds";
            default:
                return "Unknown time domain error";
        }
    }
};

namespace internal {
    constexpr TimeErrorDomain g_timeErrorDomain;
}

constexpr score::result::ErrorDomain const& GetTimeErrorDomain() noexcept {
    return internal::g_timeErrorDomain;
}

score::result::Error MakeError(const TimeErrorCode code, const std::string_view message = "")
{
    return {static_cast<score::result::ErrorCode>(code), GetTimeErrorDomain(), message};
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_ERROR_DOMAIN_H
