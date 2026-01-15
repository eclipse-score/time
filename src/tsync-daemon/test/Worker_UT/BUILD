load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "Worker_UT",
    srcs = [
        "Worker_UT.cpp",
    ],
    deps = [
        "//src/tsync-daemon:worker",
        "//src/tsync-daemon:house_keeping",
        "//src/tsync-daemon:time_base_configuration",
        "//src/tsync-utility-lib/test/matchers:test_matchers",
        "//src/tsync-utility-lib/test/mocks:utility_mocks",
        "//src/tsync-utility-lib/test/mocks:utility_tsync_shared_utils_mock",
        "//src/tsync-utility-lib:utility_tsync_id_mappings_handler",
        "//src/tsync-utility-lib:utility_tsync_named_semaphore",
        "@googletest//:gtest_main",
    ]
)
