load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "TsyncReadWriteLock_UT",
    srcs = [
        "TsyncReadWriteLock_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/tsync-utility-lib:utility_tsync_read_write_lock",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
        "@googletest//:gtest_main",
    ]
)
