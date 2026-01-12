/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/synchronized_time_base_provider.h"

#include <chrono>
#include <mutex>
#include <sstream>

#include "score/result/result.h"

#include "score/time/common/Abort.h"

#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncNamedSemaphore.h"
#include "score/time/utility/TsyncSharedUtils.h"

#include "score/time/time_error_domain.h"
#include "SynchronizedTimeBaseCommon.h"

using score::time::common::logFatalAndAbort;
using score::cpp::span;

namespace score {
namespace time {

extern TsyncIdMappingsHandler mappings_handler;

SynchronizedTimeBaseProvider::SynchronizedTimeBaseProvider(const std::string_view& instance_specifier) noexcept
    : time_base_writer_{nullptr}, time_base_reader_{nullptr}, transmission_sem_{nullptr} {
    auto domain_name = mappings_handler.GetDomainNameForProvider(instance_specifier);
    if (domain_name) {
        time_base_reader_ = TimeBaseReaderFactory::Create(*domain_name);
    } else {
        std::stringstream ss;
        ss << "SynchronizedTimeBaseProvider ctor: couldn't find time domain for provider '" << instance_specifier
           << "'";
        logFatalAndAbort(ss.str().c_str());
    }

    time_base_writer_ = TimeBaseWriterFactory::Create(*domain_name);

    // coverity[autosar_cpp14_a4_7_1_violation] Because the implementation is designed to not cause data loss here
    std::uint32_t time_domain_id{SynchronizedTimeBaseCommon::GetTimeBaseDomainId(*domain_name)};
    transmission_sem_ =
        std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(time_domain_id),
                                              TsyncNamedSemaphore::OpenMode::Unsignaled, false);
}

SynchronizedTimeBaseProvider::~SynchronizedTimeBaseProvider() noexcept = default;
SynchronizedTimeBaseProvider::SynchronizedTimeBaseProvider(SynchronizedTimeBaseProvider&& stbc) noexcept = default;
SynchronizedTimeBaseProvider& SynchronizedTimeBaseProvider::operator=(SynchronizedTimeBaseProvider&& stbp) noexcept = default;

score::ResultBlank SynchronizedTimeBaseProvider::SetTime(score::time::Timestamp timePoint,
                                                   span<const std::byte> userData) noexcept {
    score::ResultBlank result = UpdateTime(timePoint, userData);

    if (result) {
        transmission_sem_->unlock();
    }

    return result;
}

score::ResultBlank SynchronizedTimeBaseProvider::UpdateTime(score::time::Timestamp timePoint,
                                                      span<const std::byte> userData) noexcept {
    UserDataView sanitized_ud = SynchronizedTimeBaseCommon::SanitizeUserData(userData);

    auto ts = SynchronizedTimeBaseCommon::GetTimestampFromNs(timePoint.time_since_epoch());
    ts.status = SynchronizationStatus::kSynchronized;
    auto rt = TsyncSharedUtils::GetCurrentVirtualLocalTime();

    if (rt) {
        time_base_writer_->GetAccessor().Open();
        std::lock_guard<ITimeBaseWriter> lock(*time_base_writer_);
        if (time_base_writer_->Write(ts) && time_base_writer_->Write(*rt) && time_base_writer_->Write(sanitized_ud)) {
            return {};
        }
    }

    return score::MakeUnexpected<score::Blank>(score::time::TimeErrorCode::kDaemonConnectionLost);
}

score::time::Timestamp SynchronizedTimeBaseProvider::GetCurrentTime() const noexcept {
    auto ct{SynchronizedTimeBaseCommon::GetCurrentTime(*time_base_reader_)};
    if (ct) {
        return *ct;
    }

    return Timestamp(Timestamp::duration::zero());
}

score::ResultBlank SynchronizedTimeBaseProvider::SetRateCorrection(double /* rateCorrection */) noexcept {
    return {};
}

double SynchronizedTimeBaseProvider::GetRateDeviation() const noexcept {
    return 0.0;
}

score::ResultBlank SynchronizedTimeBaseProvider::SetUserData(span<const std::byte> userData) noexcept {
    time_base_writer_->GetAccessor().Open();
    {
        std::lock_guard<ITimeBaseWriter> l(*time_base_writer_);

        if (AlignedSkip<ITimeBaseWriter, TimestampWithStatus>(*time_base_writer_) &&
            AlignedSkip<ITimeBaseWriter, VirtualLocalTime>(*time_base_writer_) && time_base_writer_->Write(userData)) {
            return {};
        }
    }

    return score::MakeUnexpected<score::Blank>(score::time::TimeErrorCode::kDaemonConnectionLost);
}

span<const std::byte> SynchronizedTimeBaseProvider::GetUserData() const noexcept {
    auto ud{SynchronizedTimeBaseCommon::GetUserData(*time_base_reader_)};

    if (ud.has_value()) {
        return *ud;
    }

    return {};
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseProvider::RegisterTimeValidationNotification(
    ProviderTimeBaseValidationNotification&) noexcept {
}

// coverity[autosar_cpp14_m0_1_8_violation] This function is yet to be implemented.
void SynchronizedTimeBaseProvider::UnregisterTimeValidationNotification() noexcept {
}

}  // namespace time
}  // namespace score
