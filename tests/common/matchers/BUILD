load("@rules_cc//cc:defs.bzl", "cc_library")
# load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "test_matchers",
    hdrs = [
        "matcher_operators.h"
    ],
    includes = [
        "."
    ],
    deps = [
        "//src/tsync-ptp-lib:tsync_ptp",
        "//src/tsync-daemon:tsync_daemon",
        "//src/tsync-lib:tsync",
        "//src/tsync-utility-lib:tsync_utility",
        "//src/flatcfg:flatcfg",
        "@googletest//:gtest_main"
    ],
    visibility = ["//visibility:public"]
)