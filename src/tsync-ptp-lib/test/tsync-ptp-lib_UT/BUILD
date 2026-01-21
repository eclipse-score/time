load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "tsync-ptp-lib_UT",
    srcs = [
        "tsync-ptp-lib_UT.cpp",
    ],
    deps = [
        "//src/tsync-ptp-lib",
        "//src/tsync-utility-lib:utility_tsync_id_mappings_handler",
        "//src/tsync-utility-lib:utility_tsync_shared_utils",
        "//src/tsync-utility-lib:utility_tsync_named_semaphore",
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-lib/test/matchers:test_matchers",
        "@googletest//:gtest_main"
    ]
)
