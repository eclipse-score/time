/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include "score/time/time_error_domain.h"

using namespace score::time;

class TimeErrorDomainTestFixture : public ::testing::Test {
   protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

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
                                           std::make_tuple(TimeErrorCode::kLimitsExceeded, "value out of bounds"),
                                           std::make_tuple(static_cast<TimeErrorCode>(0xFF), "unknown tsync error")));

namespace testing {
namespace lib_tsyncerrordomain_ut {

// Test whether Name return correct name of TSync domain.
TEST_F(TimeErrorDomainTestFixture, Name_SucessCase_ReturnCorrectName) {
    // Arrange
    char expected_name[] = "tsync";

    // Act
    auto res = time_error_domain_.Name();

    // Assert
    ASSERT_TRUE(0 == memcmp(res, expected_name, sizeof(expected_name)));
}

// Test whether Message return correct string for error code. In case if error code is invalid, it would return that the
// error code is unknown.
TEST_P(TimeErrorDomainMessageTestFixture, Message_DifferentErrorCodes_RetrunCorrectDescription) {
    // Arrange
    auto parameters = GetParam();
    auto& error_code = std::get<0>(parameters);
    auto& expected_result = std::get<1>(parameters);

    // Act
    auto message = time_error_domain_.Message(static_cast<score::result::ErrorDomain::CodeType>(error_code));

    // Assert
    ASSERT_EQ(message, expected_result);
}

}  // namespace lib_tsyncerrordomain_ut
}  // namespace testing
