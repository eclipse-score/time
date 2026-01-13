/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#define private public
#include "ArgParser.h"
#undef private

using namespace score::time::daemon;

namespace testing {
namespace daemon_argparser_ut {

TEST(Daemon_ArgParser, ArgParser_WithEmptyArgs_IsCorrectlyInitialized) {
    int argc = 2;
    const char* const argv[] = {"one", "two"};

    ArgParser argParser(argc, argv);

    EXPECT_EQ(argParser.args_.size(), argc);

    int i = 0;
    for(auto arg : argParser.args_) {
        EXPECT_EQ(arg, argv[i++]);
    }
}

TEST(Daemon_ArgParser, IsDebugEnabled_WithEmptyArgs_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", ""};

    ArgParser argParser(argc, argv);
    EXPECT_FALSE(argParser.IsDebugEnabled());
}

TEST(Daemon_ArgParser, IsHelpRequested_WithEmptyArgs_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", ""};

    ArgParser argParser(argc, argv);
    EXPECT_FALSE(argParser.IsHelpRequested());
}

TEST(Daemon_ArgParser, IsHelpRequested_WithHelpArg_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", "-h"};
    ArgParser argParser(argc, argv);
    EXPECT_TRUE(argParser.IsHelpRequested());
}

TEST(Daemon_ArgParser, IsDebugEnabled_WithDebugArg_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", "-d"};
    ArgParser argParser(argc, argv);
    EXPECT_TRUE(argParser.IsDebugEnabled());
}

TEST(Daemon_ArgParser, IsVersionRequested_WithVersionArg_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", "-v"};

    ArgParser argParser(argc, argv);
    EXPECT_TRUE(argParser.IsVersionRequested());
}

TEST(Daemon_ArgParser, IsVersionRequested_WithLongVersionArg_ReportsCorrectFlagState) {
    int argc = 2;
    const char* const argv[] = {"", "--version"};

    ArgParser argParser(argc, argv);
    EXPECT_TRUE(argParser.IsVersionRequested());
}

TEST(Daemon_ArgParser, PrintHelp_Succeeds) {
    ArgParser::PrintHelp();
}

}  // namespace daemon_argparser_ut
}  // namespace testing
