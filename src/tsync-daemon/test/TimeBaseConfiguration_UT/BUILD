load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "TimeBaseConfiguration_UT",
    srcs = [
        "TimeBaseConfiguration_UT.cpp",
    ],
    deps = [
        "//src/tsync-daemon:time_base_configuration",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "@googletest//:gtest_main",
    ]
)
