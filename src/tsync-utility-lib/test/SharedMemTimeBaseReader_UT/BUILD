load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SharedMemTimeBaseReader_UT",
    srcs = [
        "SharedMemTimeBaseReader_UT.cpp",
    ],
    deps = [
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/tsync-utility-lib:utility_shared_mem_time_base_reader",
        "//src/tsync-utility-lib:utility_shared_mem_time_base_writer",
        "//src/tsync-utility-lib:utility_tsync_read_write_lock",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
        "@googletest//:gtest_main",
    ]
)
