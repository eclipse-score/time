load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "TimeErrorDomain_UT",
    srcs = [
        "TimeErrorDomain_UT.cpp",
    ],
    deps = [
        "@score_baselibs//score/result",
        "//src/tsync-lib:tsync_if",
        "@googletest//:gtest_main",
    ]
)
