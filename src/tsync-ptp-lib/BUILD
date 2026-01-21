load("@rules_cc//cc:defs.bzl", "cc_library")

# Interface (Public)
cc_library(
    name = "interface",
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
# Internal Components (Private/Subpackage visibility)
cc_library(
    name = "config",
    srcs = [
        "src/tsync_config.cpp",
    ],
    deps = [
        ":interface",
        ":data_broker",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common:common_if",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "data_broker",
    srcs = [
        "src/tsync_data_broker.cpp",
    ],
    hdrs = [
        "src/tsync_data_broker.h",
    ],
    strip_include_prefix = "src",
    deps = [
        ":interface",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common:common_if",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

cc_library(
    name = "impl",
    srcs = [
        "src/tsync_ptp_lib.cpp",
    ],
    deps = [
        ":interface",
        ":data_broker",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common:common_if",
    ],
    visibility = [
        "//src:__subpackages__",
    ],
)

#Main Library
cc_library(
    name = "tsync-ptp-lib",
    deps = [
        ":interface",
        ":config",
        ":data_broker",
        ":impl",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common:common_if",
    ],
    visibility = ["//visibility:public"],
)
