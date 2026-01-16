/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "score/time/time_error_domain.h"

using namespace score::time;

class TimeErrorDomainTestFixture : public ::testing::Test {
protected:
    TimeErrorDomain time_error_domain_;
};

class TimeErrorDomainMessageTestFixture : public TimeErrorDomainTestFixture,
                                           public ::testing::WithParamInterface<std::tuple<TimeErrorCode, std::string>> {};

// Considered test cases:
// kDaemonConnectionLost - returns message about lost connection.
// kTimeCannotSet - returns message that time can't be set.
// kLimitsExceeded - returns message that value out of bounds.
// Any error code which is not part of enum - returns that error code is unknown.
INSTANTIATE_TEST_SUITE_P(TimeErrorDomainMessageTests, TimeErrorDomainMessageTestFixture,
                         ::testing::Values(std::make_tuple(TimeErrorCode::kDaemonConnectionLost, "Daemon connection lost"),
                                           std::make_tuple(TimeErrorCode::kTimeCannotSet, "Time cannot be set"),
                                           std::make_tuple(TimeErrorCode::kLimitsExceeded, "Value out of bounds"),
                                           std::make_tuple(static_cast<TimeErrorCode>(0xFF), "Unknown time error code")));

namespace testing {
namespace tsyncerrordomain_ut {

// Test whether Message return correct string for error code. In case if error code is invalid, it would return that the
// error code is unknown.
TEST_P(TimeErrorDomainMessageTestFixture, MessageFor_DifferentErrorCodes_RetrunCorrectDescription) {
    // Arrange
    auto parameters = GetParam();
    auto& error_code = std::get<0>(parameters);
    auto& expected_result = std::get<1>(parameters);

    // Act
    auto message = time_error_domain_.MessageFor(static_cast<score::result::ErrorCode>(error_code));

    // Assert
    ASSERT_EQ(message, expected_result);
}

TEST(TimeErrorDomainTest, GetTimeErrorDomain__NoExceptionOrCrashNorNullptr) {
    // Act
    const score::result::ErrorDomain& time_error_domain = GetTimeErrorDomain();

    // Assert
    EXPECT_NE(&time_error_domain, nullptr);
}

TEST(TimeErrorDomainTest, MakeError__ErrorCreatedWithCorrectContents) {
    // Act
    score::result::Error error = MakeError(TimeErrorCode::kDaemonConnectionLost, "Test message");

    // Assert
    EXPECT_EQ(static_cast<score::result::ErrorCode>(TimeErrorCode::kDaemonConnectionLost), *error);
    EXPECT_EQ("Daemon connection lost", error.Message());
    EXPECT_EQ("Test message", error.UserMessage());
}

}  // namespace tsyncerrordomain_ut
}  // namespace testing
