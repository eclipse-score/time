load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "TsyncNamedSemaphore_UT",
    srcs = [
        "TsyncNamedSemaphore_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/tsync-utility-lib:utility_tsync_named_semaphore",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
        "@googletest//:gtest_main",
    ]
)
