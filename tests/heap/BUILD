load("@rules_cc//cc:defs.bzl", "cc_library")
# load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "test_heap",
    srcs = glob([
        "**/*.cpp"
    ]),
    hdrs = glob([
        "**/*.h"
    ]),
    deps = [
        "//include:score_headers",
    ],
    # This ensures that includes like "daemon/SCoreHouseKeeping.h" resolve correctly
    includes = [
        ".", # For Daemon_SCoreHouseKeeping_UT.cpp to find its own header (if needed)
        # Add any other include paths required for your actual source code
    ],
    visibility = ["//visibility:public"]
)