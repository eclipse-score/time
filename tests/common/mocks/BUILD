load("@rules_cc//cc:defs.bzl", "cc_library")
# load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "test_common_mocks",
    srcs = glob([
        "src/*.cpp",
        "src/**/*.cpp"
    ]),
    hdrs = glob([
        "include/**/*.h"
    ]),
    deps = [
        "//src/tsync-ptp-lib:tsync_ptp",
        "//src/tsync-daemon:tsync_daemon",
        "//src/tsync-lib:tsync",
        "//src/tsync-utility-lib:tsync_utility",
        "//src/flatcfg:flatcfg",
        "//tests/common/matchers:test_matchers",
        "//src/tsync-daemon:tsync_daemon_lib",
        "@googletest//:gtest_main"
    ],
    # This ensures that includes like "daemon/SCoreHouseKeeping.h" resolve correctly
    includes = [
        ".", # For Daemon_SCoreHouseKeeping_UT.cpp to find its own header (if needed)
        # Add any other include paths required for your actual source code
        "include/"
    ],
    strip_include_prefix = "include",
    visibility = [
        "//visibility:public",
        "//tests/SCORE_UT/Lib_SharedMemTimeBaseReader_UT:__package__"
    ],
)
