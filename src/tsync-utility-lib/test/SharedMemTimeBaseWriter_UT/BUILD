load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SharedMemTimeBaseWriter_UT",
    srcs = [
        "SharedMemTimeBaseWriter_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
        "@googletest//:gtest_main",
    ]
)
