load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SynchronizedTimeBaseStatus_UT",
    srcs = [
        "SynchronizedTimeBaseStatus_UT.cpp",
    ],
    deps = [
        "//src/tsync-lib:tsync_if",
        "//src/tsync-lib:synchronized_time_base_status_impl",
        "@googletest//:gtest_main",
    ]
)
