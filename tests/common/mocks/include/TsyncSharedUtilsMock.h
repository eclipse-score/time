/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef TSYNC_SHARED_UTILS_MOCK_H_
#define TSYNC_SHARED_UTILS_MOCK_H_

#include <memory>
#include <string>

#include <gmock/gmock.h>

#include "score/time/utility/TsyncNamedSemaphore.h"

namespace score {
namespace time {

class TsyncSharedUtilsMock {
public:
    MOCK_METHOD1(GetTransmissionSemaphoreName, std::string(std::uint32_t));
    MOCK_METHOD2(CreateTransmissionSemaphore, TsyncNamedSemaphore(std::uint32_t, bool));
    MOCK_METHOD0(GetCurrentVirtualLocalTime, std::optional<std::chrono::nanoseconds>());

    TsyncSharedUtilsMock() {
        ON_CALL(*this, CreateTransmissionSemaphore).WillByDefault([](std::uint32_t domain_id, bool is_owner) {
            return TsyncNamedSemaphore(
                std::string("time_domain_") + std::to_string(static_cast<std::int32_t>(domain_id)),
                TsyncNamedSemaphore::OpenMode::Unsignaled, is_owner);
        });
        ON_CALL(*this, GetCurrentVirtualLocalTime).WillByDefault(testing::Return(std::nullopt));
    }
};

extern std::unique_ptr<::testing::NiceMock<TsyncSharedUtilsMock>> shared_utils_mock;

inline std::string TsyncSharedUtils::GetTransmissionSemaphoreName(std::uint32_t time_domain_id) {
    return shared_utils_mock->GetTransmissionSemaphoreName(time_domain_id);
}

inline TsyncNamedSemaphore TsyncSharedUtils::CreateTransmissionSemaphore(std::uint32_t time_domain_id, bool as_owner) {
    return shared_utils_mock->CreateTransmissionSemaphore(time_domain_id, as_owner);
}

inline std::optional<std::chrono::nanoseconds> TsyncSharedUtils::GetCurrentVirtualLocalTime() {
    return shared_utils_mock->GetCurrentVirtualLocalTime();
}

}  // namespace time
}  // namespace score

#endif
