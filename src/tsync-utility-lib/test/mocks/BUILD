load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "utility_mocks",
    srcs = [
        "SysCallsReadWriteLockMock.cpp",
        "SysCallsShMemMock.cpp",
    ],
    hdrs = [
        "SysCallsReadWriteLockMock.h",
        "SysCallsShMemMock.h",
    ],
    strip_include_prefix = ".",
    deps = [
        "@googletest//:gtest",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)
