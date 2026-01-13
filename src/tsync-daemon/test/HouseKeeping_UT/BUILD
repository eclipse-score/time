load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "HouseKeeping_UT",
    srcs = [
        "HouseKeeping_UT.cpp",
    ],
    deps = [
        "//src/tsync-daemon:house_keeping",
        "@googletest//:gtest_main",
    ]
)
