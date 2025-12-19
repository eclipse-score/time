/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <score/time/timestamp.h>
#include <gtest/gtest.h>

#include <type_traits>

using namespace score::time;

namespace testing {
namespace lib_timestampexistence_ut {

TEST(Lib_TimeStamp_Existence_UT, TypeExists) {
    ASSERT_TRUE(std::is_constructible<Timestamp>::value);
}

}  // namespace lib_timestampexistence_ut
}  // namespace testing