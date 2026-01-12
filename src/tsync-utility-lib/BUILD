load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tsync_utility_if",
    hdrs = glob([
        "include/**/*.h"
    ]),
    strip_include_prefix = "include",
    deps = [
        "//src/tsync-lib:tsync_if",
        "@score_baselibs//score/language/futurecpp:futurecpp",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "utility_shared_mem_time_base_reader",
    srcs = [
        "src/SharedMemTimeBaseReader.cpp",
    ],
    hdrs = [
        "src/SharedMemLayout.h",
        "src/SharedMemTimeBaseReader.h",
        "src/TsyncReadWriteLock.h",
    ],
    strip_include_prefix = "src",
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_shared_mem_time_base_writer",
    srcs = [
        "src/SharedMemTimeBaseWriter.cpp",
    ],
    hdrs = [
        "src/SharedMemLayout.h",
        "src/SharedMemTimeBaseWriter.h",
        "src/TsyncReadWriteLock.h",
    ],
    strip_include_prefix = "src",
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_sys_calls",
    srcs = ["src/SysCalls.cpp"],
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_time_base_reader_factory",
    srcs = [
        "src/SharedMemTimeBaseReader.h",
        "src/TimeBaseReaderFactory.cpp",
        "src/TsyncReadWriteLock.h",
    ],
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_time_base_writer_factory",
    srcs = [
        "src/SharedMemTimeBaseWriter.h",
        "src/TimeBaseWriterFactory.cpp",
        "src/TsyncReadWriteLock.h",
    ],
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_tsync_id_mappings_handler",
    srcs = ["src/TsyncIdMappingsHandler.cpp"],
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_tsync_named_semaphore",
    srcs = ["src/TsyncNamedSemaphore.cpp"],
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_tsync_read_write_lock",
    srcs = ["src/TsyncReadWriteLock.cpp"],
    hdrs = ["src/TsyncReadWriteLock.h"],
    strip_include_prefix = "src",
    deps = [
        ":tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "utility_tsync_shared_utils",
    srcs = ["src/TsyncSharedUtils.cpp"],
    deps = [
        ":tsync_utility_if",
        "//src/common",
        "@score_baselibs//score/language/futurecpp",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "tsync_utility",
    hdrs = [
        ":tsync_utility_if",
    ],
    srcs = [
        ":utility_time_base_reader_factory",
        ":utility_time_base_writer_factory",
        ":utility_shared_mem_time_base_reader",
        ":utility_shared_mem_time_base_writer",
        ":utility_sys_calls",
        ":utility_tsync_id_mappings_handler",
        ":utility_tsync_named_semaphore",
        ":utility_tsync_read_write_lock",
        ":utility_tsync_shared_utils",
    ],
    deps = [
        ":tsync_utility_if",
    ],
    # linkstatic = True,
    visibility = [
        "//src:__subpackages__",
        "//tests:__subpackages__",
    ],
)
