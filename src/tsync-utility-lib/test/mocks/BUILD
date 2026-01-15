load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "utility_mocks",
    srcs = [
        "SysCallsMiscMock.cpp",
        "SysCallsNamedSemMock.cpp",
        "SysCallsReadWriteLockMock.cpp",
        "SysCallsShMemMock.cpp",
        "TimeBaseReaderFactoryMock.cpp",
        "TimeBaseWriterFactoryMock.cpp",
    ],
    hdrs = [
        "SharedMemTimeBaseReaderMock.h",
        "SharedMemTimeBaseWriterMock.h",
        "SysCallsMiscMock.h",
        "SysCallsNamedSemMock.h",
        "SysCallsReadWriteLockMock.h",
        "SysCallsShMemMock.h",
        "TimeBaseReaderFactoryMock.h",
        "TimeBaseWriterFactoryMock.h",
        # "TsyncSharedUtilsMock.h",
    ],
    strip_include_prefix = ".",
    deps = [
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/tsync-utility-lib:utility_tsync_read_write_lock",
        "//src/tsync-utility-lib:utility_shared_mem_time_base_reader",
        "//src/tsync-utility-lib:utility_shared_mem_time_base_writer",
        "@googletest//:gtest",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_tsync_shared_utils_mock",
    srcs = [
        "TsyncSharedUtilsMock.cpp",
    ],
    hdrs = [
        "TsyncSharedUtilsMock.h",
    ],
    strip_include_prefix = ".",
    deps = [
        "//src/tsync-utility-lib:tsync_utility_if",
        "@googletest//:gtest",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)
