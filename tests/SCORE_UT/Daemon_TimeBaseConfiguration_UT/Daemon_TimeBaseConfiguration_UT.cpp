/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <mutex>
#include <string_view>

#include "matcher_operators.h"

#define private public
// class under test
#include "TimeBaseConfiguration.h"
#undef private

using namespace score::time;
using InstanceSpecifier = std::string_view;

static const char* CONFIG_DATA_NAME = "TestConfig";

class TimeBaseConfigurationSizeTestFixture
    : public ::testing::TestWithParam<std::tuple<std::vector<TimeBaseConfigData>, int>> {};

// Considered test cases:
// No configuration added - returns size 0.
// One configuration added - returns size 1.
// Two different configurations added - returns size 2.
// Two equal configurations added - returns size 1.
INSTANTIATE_TEST_SUITE_P(
    TimeBaseConfigurationSize, TimeBaseConfigurationSizeTestFixture,
    ::testing::Values(
        std::make_tuple(std::vector<TimeBaseConfigData>{}, 0), std::make_tuple(std::vector<TimeBaseConfigData>{{}}, 1),
        std::make_tuple(std::vector<TimeBaseConfigData>{{"1", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}},
                                                        {"2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}}},
                        2),
        std::make_tuple(std::vector<TimeBaseConfigData>{{"1", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}},
                                                        {"1", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}}},
                        1)));

namespace testing {
namespace daemon_timebaseconfiguration_ut {

TEST(Daemon_TimeBaseConfiguration_UT, AddConfigData) {
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    TimeBaseConfigData data;
    data.timebase_name = CONFIG_DATA_NAME;
    instance.AddConfigData(data);

    auto opt = instance.GetConfigData(CONFIG_DATA_NAME);
    ASSERT_TRUE(opt);
    auto retrieved_data = *opt;

    EXPECT_EQ(data.timebase_name, retrieved_data.timebase_name);
    EXPECT_EQ(data.timebase_config, retrieved_data.timebase_config);
}

TEST(Daemon_TimeBaseConfiguration_UT, AddMoreConfigData) {
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    TimeBaseConfigData data;
    data.timebase_name = CONFIG_DATA_NAME;
    data.timebase_config.domain_id = 1;
    instance.AddConfigData(data);

    auto opt = instance.GetConfigData(CONFIG_DATA_NAME);
    ASSERT_TRUE(opt);
    auto retrieved_data = *opt;

    EXPECT_EQ(data.timebase_name, retrieved_data.timebase_name);
    EXPECT_EQ(data.timebase_config, retrieved_data.timebase_config);
}

// Note: This test is not strictly required due to the singleton nature
// of TimeBaseConfiguration. It is covered by the other tests. Better
// to be explicit, though.
TEST(Daemon_TimeBaseConfiguration_UT, Replace) {
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    TimeBaseConfigData data;
    data.timebase_name = CONFIG_DATA_NAME;
    data.timebase_config.domain_id = 1;
    instance.AddConfigData(data);

    auto opt = instance.GetConfigData(CONFIG_DATA_NAME);
    ASSERT_TRUE(opt);
    auto retrieved_data = *opt;

    EXPECT_EQ(data.timebase_name, retrieved_data.timebase_name);
    EXPECT_EQ(data.timebase_config, retrieved_data.timebase_config);

    data.timebase_config.domain_id = 2;
    instance.AddConfigData(data);

    opt = instance.GetConfigData(CONFIG_DATA_NAME);
    ASSERT_TRUE(opt);
    retrieved_data = *opt;

    EXPECT_EQ(data.timebase_name, retrieved_data.timebase_name);
    EXPECT_EQ(data.timebase_config, retrieved_data.timebase_config);
}

// Test whether in case if no configuration was added that begin (cbegin) and
// end (cend) iterators are equal.
TEST(Daemon_TimeBaseConfiguration_UT, Begin_End_ConfigurationEmpty_AreEqual) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();

    // Act
    auto begin_it = instance.begin();
    auto end_it = instance.end();
    auto cbegin_it = instance.cbegin();
    auto cend_it = instance.cend();

    // Assert
    ASSERT_EQ(begin_it, end_it);
    ASSERT_EQ(cbegin_it, cend_it);
}

// Test whether in case if there is configuration was added, the distance between begin (cbegin)
// and end (cend) is equal to amount of configurations.
TEST(Daemon_TimeBaseConfiguration_UT, Begin_End_OneConfig_IteratorsValid) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();

    TimeBaseConfigData data;
    instance.AddConfigData(data);

    // Act
    auto begin_it = instance.begin();
    auto end_it = instance.end();
    auto cbegin_it = instance.cbegin();
    auto cend_it = instance.cend();

    // Assert
    ASSERT_EQ(std::distance(begin_it, end_it), 1);
    ASSERT_EQ(std::distance(cbegin_it, cend_it), 1);
}

// Test whether if there is no configuration was added, GetConfigData will return
// default configuration for any name
TEST(Daemon_TimeBaseConfiguration_UT, GetConfigData_NonExistingName_ReturnsDefaultConfiguration) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();

    // Act
    auto opt = instance.GetConfigData("1");
    ASSERT_FALSE(opt);
}

// Test whether if there is no configuration was added, GetConfigData will return
// default configuration for any instance specifier
TEST(Daemon_TimeBaseConfiguration_UT, GetConfigData_NonExistingInstanceSpecifier_ReturnsDefaultConfiguration) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();

    InstanceSpecifier config_specifier("mytest/config");

    // Act
    auto res = instance.GetConfigData(config_specifier);

    // Assert
    ASSERT_FALSE(res);
}

// Test whether in case if there is configuration was added, GetConfigData will return
// correct configuration by InstanceSpecifier
TEST(Daemon_TimeBaseConfiguration_UT, GetConfigData_ByInstanceSpecifier_ReturnsCorrectConfiguration) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();
    InstanceSpecifier config_specifier("mytest/config");
    TimeBaseConfigData expected_config{config_specifier,
                                       TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}};

    instance.AddConfigData(expected_config);

    // Act
    auto opt = instance.GetConfigData(config_specifier);
    ASSERT_TRUE(opt);
    auto config = *opt;

    // Assert
    ASSERT_EQ(config.timebase_name, expected_config.timebase_name);
    ASSERT_EQ(config.timebase_config, expected_config.timebase_config);
}

// Test that size return correct value based on amount of added configurations.
TEST_P(TimeBaseConfigurationSizeTestFixture, Size_DifferentConfigs_ReturnsCorrectValue) {
    // Arrange
    TimeBaseConfiguration& instance = TimeBaseConfiguration::GetInstance();
    instance.Clear();

    auto parameters = GetParam();
    auto& configs = std::get<0>(parameters);
    auto& expected_result = std::get<1>(parameters);

    for (auto& config : configs) {
        instance.AddConfigData(config);
    }

    // Act
    auto size = instance.size();

    // Assert
    ASSERT_EQ(size, expected_result);
}

}  // namespace daemon_timebaseconfiguration_ut
}  // namespace testing
