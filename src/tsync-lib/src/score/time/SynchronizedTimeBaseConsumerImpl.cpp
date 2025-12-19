/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/synchronized_time_base_consumer.h"

#include <chrono>
#include <sstream>

#include "score/time/common/Abort.h"

#include "score/time/utility/ITimeBaseAccessor.h"
#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncSharedUtils.h"

#include "score/time/SynchronizedTimeBaseCommon.h"
#include "score/time/TimeBaseStatusAccessMediator.h"

using namespace std::chrono;
using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

extern TsyncIdMappingsHandler mappings_handler;

SynchronizedTimeBaseConsumer::SynchronizedTimeBaseConsumer(const std::string_view& instance_specifier) noexcept {
    auto domain_name = mappings_handler.GetDomainNameForConsumer(instance_specifier);

    if (domain_name) {
        time_base_reader_ = TimeBaseReaderFactory::Create(*domain_name);
    } else {
        std::stringstream ss;
        ss << "SynchronizedTimeBaseConsumer ctor: couldn't find time domain for consumer '" << instance_specifier
           << "'";
        logFatalAndAbort(ss.str().c_str());
    }
}

SynchronizedTimeBaseConsumer::~SynchronizedTimeBaseConsumer() noexcept = default;
SynchronizedTimeBaseConsumer::SynchronizedTimeBaseConsumer(SynchronizedTimeBaseConsumer&& stbc) noexcept = default;
SynchronizedTimeBaseConsumer& SynchronizedTimeBaseConsumer::operator=(SynchronizedTimeBaseConsumer&& stbc) & noexcept = default;

Timestamp SynchronizedTimeBaseConsumer::GetCurrentTime() const noexcept {
    auto sync_status = SynchronizedTimeBaseCommon::GetSynchronizationStatus(*time_base_reader_);
    if (!sync_status || *sync_status == SynchronizationStatus::kNotSynchronizedUntilStartup) {
        auto vlt = TsyncSharedUtils::GetCurrentVirtualLocalTime();
        if (!vlt) {
            return Timestamp(Timestamp::duration::zero());
        } else {
            return Timestamp(*vlt);
        }
    }

    auto ct = SynchronizedTimeBaseCommon::GetCurrentTime(*time_base_reader_);
    if (ct) {
        return *ct;
    }

    return Timestamp(Timestamp::duration::zero());
}

SynchronizedTimeBaseStatus SynchronizedTimeBaseConsumer::GetTimeWithStatus() const noexcept {
    SynchronizedTimeBaseStatus status = TimeBaseStatusAccessMediator::CreateSynchronizedTimeBaseStatusInstance();

    auto sync_status{SynchronizedTimeBaseCommon::GetSynchronizationStatus(*time_base_reader_)};
    Timestamp ts_now{};

    if (!sync_status.has_value() || *sync_status == SynchronizationStatus::kNotSynchronizedUntilStartup) {
        sync_status = SynchronizationStatus::kNotSynchronizedUntilStartup;
        auto rs = TsyncSharedUtils::GetCurrentVirtualLocalTime();
        if (!rs) {
            ts_now = Timestamp(Timestamp::duration::zero());
        } else {
            ts_now = Timestamp(*rs);
        }
    } else {
        auto ct = SynchronizedTimeBaseCommon::GetCurrentTime(*time_base_reader_);
        if (ct.has_value()) {
            ts_now = *ct;
        }
    }

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusCreationTime(status, ts_now);
    // Get user data from shared memory
    auto ud{SynchronizedTimeBaseCommon::GetUserData(*time_base_reader_)};
    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusUserData(status, ud.has_value() ? *ud : UserDataView());

    TimeBaseStatusAccessMediator::SetSynchronizedTimeBaseStatusSynchronizationStatus(status, *sync_status);

    return status;
}

double SynchronizedTimeBaseConsumer::GetRateDeviation() const noexcept {
    return 0.0;
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::RegisterStatusChangeNotifier(
    std::function<void(const SynchronizedTimeBaseStatus&)> /* notifier */) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::UnregisterStatusChangeNotifier() noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::RegisterSynchronizationStateChangeNotifier(
    std::function<void(const SynchronizationStatus&)> /* notifier */) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::UnregisterSynchronizationStateChangeNotifier() noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::RegisterTimeLeapNotifier(
    std::function<void(const SynchronizedTimeBaseStatus&)> /* notifier */) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::UnregisterTimeLeapNotifier() noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::RegisterTimeValidationNotification(
    ConsumerTimeBaseValidationNotification& /* timeBaseValidationNotification */) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::UnregisterTimeValidationNotification() noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::RegisterTimePrecisionMeasurementNotifier(
    std::function<void(const TimePrecisionMeasurement&)> /* notifier */) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseConsumer::UnregisterTimePrecisionMeasurementNotifier() noexcept {
}

}  // namespace time
}  // namespace score
