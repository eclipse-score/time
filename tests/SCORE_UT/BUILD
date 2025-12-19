load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "score_ut",
    srcs = glob(
        ["**/*.cpp"]
    ) + [
        "//src/score/time/daemon:src/ConfigLoader.cpp",
        "//src/score/time/daemon:src/tsyncd_main.cpp"
    ],
    includes = [
        "src/score/time/daemon/src",
        "tests/common/matchers",
        "tests/common/mocks",
        "include"
      ],
    data = [
        "//src/score/time/daemon:tsync_daemon"
    ],
    deps = [
        "//tests/common/matchers:test_matchers",
        "//tests/common/mocks:test_common_mocks",
        "//src/score/time/daemon:tsync_daemon_src_lib",
        "//src/flatcfg:tsync_flatcfg",
        "//include:score_headers",
        "@googletest//:gtest",
        "@googletest//:gtest_main"
    ]
)