load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SynchronizedTimeBaseProvider_UT",
    srcs = [
        "SynchronizedTimeBaseProvider_UT.cpp",
    ],
    deps = [
        "//src/tsync-lib:tsync_if",
        "//src/tsync-lib:synchronized_time_base_common",
        "//src/tsync-lib:synchronized_time_base_provider_impl",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/mocks:utility_tsync_shared_utils_mock",
        "//src/tsync-utility-lib:utility_tsync_id_mappings_handler",
        "//src/tsync-utility-lib:utility_tsync_named_semaphore",
        "@googletest//:gtest_main",
    ]
)
