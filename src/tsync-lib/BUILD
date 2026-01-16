load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tsync_if",
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
    deps = [
        "@score_baselibs//score/result",
        "@score_baselibs//score/language/futurecpp:futurecpp",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "synchronized_time_base_common",
    srcs = [
        "src/SynchronizedTimeBaseCommon.cpp"
    ],
    hdrs = ["src/SynchronizedTimeBaseCommon.h"],
    strip_include_prefix = "src",
    deps = [
        ":tsync_if",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common",
        "@score_baselibs//score/language/futurecpp:futurecpp",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "synchronized_time_base_consumer_impl",
    srcs = [
        "src/SynchronizedTimeBaseCommon.h",
        "src/SynchronizedTimeBaseConsumerImpl.cpp",
        "src/TimeBaseStatusAccessMediator.h",
    ],
    deps = [
        ":tsync_if",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "synchronized_time_base_provider_impl",
    srcs = [
        "src/SynchronizedTimeBaseCommon.h",
        "src/SynchronizedTimeBaseProviderImpl.cpp",
    ],
    deps = [
        ":tsync_if",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common",
        "@score_baselibs//score/result",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "synchronized_time_base_status_impl",
    srcs = [
        "src/SynchronizedTimeBaseStatusImpl.cpp",
    ],
    hdrs = [
        "src/TimeBaseStatusAccessMediator.h",
    ],
    strip_include_prefix = "src",
    deps = [
        ":tsync_if",
        "//src/tsync-utility-lib:tsync_utility_if",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "tsync",
    hdrs = [
        ":tsync_if",
    ],
    srcs = [
        ":synchronized_time_base_consumer_impl",
        ":synchronized_time_base_provider_impl",
        ":synchronized_time_base_common",
        ":synchronized_time_base_status_impl",
    ],
    deps = [
        ":tsync_if",
        "//src/tsync-utility-lib:tsync_utility",
        "//src/common",
        #"@score_baselibs//score/mw/log",
        "@score_baselibs//score/language/futurecpp:futurecpp",
    ],
    visibility = ["//visibility:public"],
)
