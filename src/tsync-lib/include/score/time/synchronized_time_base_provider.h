/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYNCHRONIZED_TIME_BASE_PROVIDER_H_
#define SCORE_TIME_SYNCHRONIZED_TIME_BASE_PROVIDER_H_

#include <memory>
#include <string_view>

#include "score/span.hpp"
#include "score/result/result.h"

#include "score/time/provider_time_base_validation_notification.h"
#include "score/time/timestamp.h"

namespace score {
namespace time {

class ITimeBaseWriter;
class ITimeBaseReader;
class TsyncNamedSemaphore;

/**
 * @traceid{SWS_TS_01100}
 */
class SynchronizedTimeBaseProvider final {
public:
    /**
     * @param instance_specifier String representation of an score::mw::com::InstanceSpecifier to
     * a PortPrototype of a TimeSynchronizationInterface
     *
     * @traceid{SWS_TS_01101}
     */
    explicit SynchronizedTimeBaseProvider(const std::string_view& instance_specifier) noexcept;

    /**
     * @traceid{SWS_TS_01102}
     * @threadsafety{re-entrant}
     * @param[in] stbp The SynchronizedTimeBaseProvider object to be moved.
     */
    SynchronizedTimeBaseProvider(SynchronizedTimeBaseProvider&& stbp) noexcept;

    /**
     * @traceid{SWS_TS_01103}
     * @threadsafety{re-entrant}
     * @param[in] stbp The SynchronizedTimeBaseProvider object to be moved.
     * @returns The moved SynchronizedTimeBaseProvider object.
     */
    SynchronizedTimeBaseProvider& operator=(SynchronizedTimeBaseProvider&& stbp) noexcept;

    /**
     * @traceid{SWS_TS_01104}
     */
    SynchronizedTimeBaseProvider(const SynchronizedTimeBaseProvider&) = delete;

    /**
     * @traceid{SWS_TS_01105}
     */
    SynchronizedTimeBaseProvider& operator=(const SynchronizedTimeBaseProvider&) = delete;

    /**
     * @traceid{SWS_TS_01106}
     * @threadsafety{no}
     */
    ~SynchronizedTimeBaseProvider() noexcept;

    /**
     * @param[in] timePoint The time information to be set.
     * @param[in] userData The user data to be set.
     * @error TimeErrorCode::kTimeCannotSet  the action cannot be executed, because the connection to time sync daemon is
     * currently lost
     * @traceid{SWS_TS_01107}
     */
    score::ResultBlank SetTime(score::time::Timestamp timePoint,
                               score::cpp::span<const std::byte> userData = {}) noexcept;

    /**
     * @param[in] timePoint The time information to be set.
     * @param[in] userData The user data to be set.
     * @error TimeErrorCode::kDaemonConnectionLost  the action cannot be executed, because the connection to time sync
     * daemon is currently lost
     * @tparam Duration The duration type of the time point passed as parameter.
     * @traceid{SWS_TS_01108}
     */
    score::ResultBlank UpdateTime(score::time::Timestamp timePoint,
                                  score::cpp::span<const std::byte> userData = {}) noexcept;

    /**
     * @returns The current time as clock specific time point.
     * @traceid{SWS_TS_01109}
     */
    Timestamp GetCurrentTime() const noexcept;

    /**
     * @param[in] rateCorrection The rate correction to be applied. 0.5 is two times slower, whilst 2.0 is 2 times
     * faster.
     * @error score::time::TimeErrorDomain::Errc::kLimitsExceeded
     * @traceid{SWS_TS_01110}
     */
    score::ResultBlank SetRateCorrection(double rateCorrection) noexcept;

    /**
     * @returns The current rate deviation.
     * @traceid{SWS_TS_01111}
     * @uptrace{RS_TS_00018}
     */
    double GetRateDeviation() const noexcept;

    /**
     * @param[in] userData The user data to be set.
     * @traceid{SWS_TS_01112}
     */
    score::ResultBlank SetUserData(score::cpp::span<const std::byte> userData) noexcept;

    /**
     * @returns A vector of bytes holding the user data that was set during the creation of the status.
     * @traceid{SWS_TS_01113}
     */
    score::cpp::span<const std::byte> GetUserData() const noexcept;

    /**
     * @param[in] timeBaseProviderNotification time provider application notification object that will be notified about
     * the availability of a new data block recorded for the Time Base
     * @traceid{SWS_TS_01114}
     */
    void RegisterTimeValidationNotification(
        ProviderTimeBaseValidationNotification& timeBaseValidationNotification) noexcept;

    /**
     * @traceid{SWS_TS_01115}
     */
    void UnregisterTimeValidationNotification() noexcept;

private:
    std::unique_ptr<ITimeBaseWriter> time_base_writer_;
    std::unique_ptr<ITimeBaseReader> time_base_reader_;
    std::unique_ptr<TsyncNamedSemaphore> transmission_sem_;
};

} // namespace time
} // namespace score

#endif  // SCORE_TIME_SYNCHRONIZED_TIME_BASE_PROVIDER_H_
