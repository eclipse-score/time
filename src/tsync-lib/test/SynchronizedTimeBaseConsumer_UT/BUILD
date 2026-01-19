load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "SynchronizedTimeBaseConsumer_UT",
    srcs = [
        "SynchronizedTimeBaseConsumer_UT.cpp",
    ],
    deps = [
        "//src/tsync-lib:tsync_if",
        "//src/tsync-lib:synchronized_time_base_common",
        "//src/tsync-lib:synchronized_time_base_consumer_impl",
        "//src/tsync-lib:synchronized_time_base_status_impl",
        "//src/tsync-lib/test/matchers:test_matchers",
        "//src/tsync-lib/test/mocks:mocks",
        "//src/tsync-utility-lib:utility_tsync_id_mappings_handler",
        "//src/tsync-utility-lib:utility_tsync_named_semaphore",
        "//src/tsync-utility-lib:utility_tsync_read_write_lock",
        "//src/tsync-utility-lib:utility_sys_calls",
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/mocks:utility_tsync_shared_utils_mock",
        "@googletest//:gtest_main",
    ]
)
