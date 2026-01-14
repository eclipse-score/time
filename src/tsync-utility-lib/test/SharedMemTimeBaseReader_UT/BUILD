load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SharedMemTimeBaseReader_UT",
    srcs = [
        "SharedMemTimeBaseReader_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "@score_baselibs//score/language/futurecpp",
        "@googletest//:gtest_main",
    ]
)
