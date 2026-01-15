load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "TsyncReadWriteLock_UT",
    srcs = [
        "TsyncReadWriteLock_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/common",
        "@googletest//:gtest_main",
    ]
)
