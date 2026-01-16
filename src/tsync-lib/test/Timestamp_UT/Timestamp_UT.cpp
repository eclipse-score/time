/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <score/time/timestamp.h>
#include <gtest/gtest.h>

#include <type_traits>

using namespace score::time;

namespace testing {
namespace timestampexistence_ut {

TEST(TimeStamp_Existence_UT, TypeExists) {
    EXPECT_TRUE(std::is_constructible<Timestamp>::value);
}

}  // namespace timestampexistence_ut
}  // namespace testing
