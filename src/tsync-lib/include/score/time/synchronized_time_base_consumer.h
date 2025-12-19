/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

/// @brief Synchronized Time Base Consumer class

#ifndef SCORE_TIME_SYNCHRONIZED_TIME_BASE_CONSUMER_H_
#define SCORE_TIME_SYNCHRONIZED_TIME_BASE_CONSUMER_H_

#include <functional>
#include <memory>
#include <string_view>

#include "score/time/consumer_time_base_validation_notification.h"
#include "score/time/synchronized_time_base_status.h"
#include "score/time/time_precision_measurement_type.h"
#include "score/time/timestamp.h"

namespace score {
namespace time {

class ITimeBaseReader;

/**
 * @traceid{SWS_TS_01000}
 */
class SynchronizedTimeBaseConsumer final {
public:
    /**
     * @param instance_specifier String representation of an score::mw::com::InstanceSpecifier to 
     * an PortPrototype of an TimeSynchronizationInterface
     * @traceid{SWS_TS_01001}
     */
    explicit SynchronizedTimeBaseConsumer(const std::string_view& instance_specifier) noexcept;

    /**
     * @traceid{SWS_TS_01002}
     */
    ~SynchronizedTimeBaseConsumer() noexcept;

    /**
     * @traceid{SWS_TS_01003}
     * @threadsafety{re-entrant}
     * @param[in] stbc The SynchronizedTimeBaseConsumer object to be moved.
     */
    SynchronizedTimeBaseConsumer(SynchronizedTimeBaseConsumer&& stbc) noexcept;

    /**
     * @traceid{SWS_TS_01004}
     * @threadsafety{re-entrant}
     * @param[in] stbc The SynchronizedTimeBaseConsumer object to be moved.
     * @returns A reference to this object.
     */
    SynchronizedTimeBaseConsumer& operator=(SynchronizedTimeBaseConsumer&& stbc) & noexcept;

    /**
     * @traceid{SWS_TS_01005}
     */
    SynchronizedTimeBaseConsumer(const SynchronizedTimeBaseConsumer&) = delete;

    /**
     * @traceid{SWS_TS_01006}
     */
    SynchronizedTimeBaseConsumer& operator=(const SynchronizedTimeBaseConsumer&) = delete;

    /**
     * @returns The current time of the synchronized clock.
     * @traceid{SWS_TS_01007}
     */
    Timestamp GetCurrentTime() const noexcept;

    /**
     * @returns The current rate deviation.
     * @traceid{SWS_TS_01008}
     */
    double GetRateDeviation() const noexcept;

    /**
     * @returns A clock specific TimeBaseStatus that contains all the relevant clock information.
     * @traceid{SWS_TS_01009}
     */
    SynchronizedTimeBaseStatus GetTimeWithStatus() const noexcept;

    /**
     * @traceid{SWS_TS_01010}
     */
    void RegisterStatusChangeNotifier(std::function<void(const SynchronizedTimeBaseStatus&)> notifier) noexcept;

    /**
     * @traceid{SWS_TS_01011}
     */
    void UnregisterStatusChangeNotifier() noexcept;

    /**
     * @param[in] notifier  The function to unregister.
     * @traceid{SWS_TS_01012}
     */
    void RegisterSynchronizationStateChangeNotifier(
        std::function<void(const SynchronizationStatus&)> notifier) noexcept;

    /**
     * @traceid{SWS_TS_01013}
     */
    void UnregisterSynchronizationStateChangeNotifier() noexcept;

    /**
     * @param[in] notifier  The function to be called if the TimeBaseStatus has changed.
     * @traceid{SWS_TS_01014}
     */
    void RegisterTimeLeapNotifier(std::function<void(const SynchronizedTimeBaseStatus&)> notifier) noexcept;

    /**
     * @traceid{SWS_TS_01015}
     */
    void UnregisterTimeLeapNotifier() noexcept;

    /**
     * @param[in] timeBaseProviderNotification time consumer application notification object that will be notified about
     * the availability of a new data block recorded for the Time Base
     * @traceid{SWS_TS_01016}
     */
    void RegisterTimeValidationNotification(
        ConsumerTimeBaseValidationNotification& timeBaseValidationNotification) noexcept;

    /**
     * @traceid{SWS_TS_01017}
     */
    void UnregisterTimeValidationNotification() noexcept;

    /**
     * @param[in] notifier  The function to be called.
     * @traceid{SWS_TS_01018}
     */
    void RegisterTimePrecisionMeasurementNotifier(
        std::function<void(const TimePrecisionMeasurement&)> notifier) noexcept;

    /**
     * @traceid{SWS_TS_01019}
     */
    void UnregisterTimePrecisionMeasurementNotifier() noexcept;

private:
    std::unique_ptr<ITimeBaseReader> time_base_reader_;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYNCHRONIZED_TIME_BASE_CONSUMER_H_
