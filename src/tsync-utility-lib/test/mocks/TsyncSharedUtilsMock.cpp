/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TsyncSharedUtilsMock.h"

#include <memory>

#include <gmock/gmock.h>

#include "score/time/utility/TsyncSharedUtils.h"

namespace score {
namespace time {

std::unique_ptr<::testing::NiceMock<TsyncSharedUtilsMock>> shared_utils_mock;

std::string TsyncSharedUtils::GetTransmissionSemaphoreName(std::uint32_t time_domain_id) {
    return shared_utils_mock->GetTransmissionSemaphoreName(time_domain_id);
}

TsyncNamedSemaphore TsyncSharedUtils::CreateTransmissionSemaphore(std::uint32_t time_domain_id, bool as_owner) {
    return shared_utils_mock->CreateTransmissionSemaphore(time_domain_id, as_owner);
}

std::optional<std::chrono::nanoseconds> TsyncSharedUtils::GetCurrentVirtualLocalTime() {
    return shared_utils_mock->GetCurrentVirtualLocalTime();
}

}  // namespace time
}  // namespace score
