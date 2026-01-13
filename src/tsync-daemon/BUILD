# src/score/time/daemon/BUILD

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@flatbuffers//:build_defs.bzl", "flatbuffer_cc_library")

cc_library(
    name = "arg_parser",
    srcs = ["src/ArgParser.cpp"],
    hdrs = ["src/ArgParser.h"],
    strip_include_prefix = "src",
    visibility = [
        ":__subpackages__",
    ],
)

cc_library(
    name = "house_keeping",
    srcs = ["src/HouseKeeping.cpp"],
    hdrs = ["src/HouseKeeping.h"],
    strip_include_prefix = "src",
    visibility = [
        ":__subpackages__",
    ],
)

cc_library(
    name = "time_base_configuration",
    srcs = ["src/TimeBaseConfiguration.cpp"],
    hdrs = ["src/TimeBaseConfiguration.h"],
    strip_include_prefix = "src",
    deps = [
        "//src/tsync-utility-lib:tsync_utility_if",
    ],
    visibility = [
        ":__subpackages__",
    ],
)

cc_library(
    name = "worker",
    srcs = [
        "src/ConfigLoader.h",
        "src/HouseKeeping.h",
        "src/TimeBaseConfiguration.h",
        "src/TsyncWorker.cpp"
    ],
    hdrs = ["src/TsyncWorker.h"],
    strip_include_prefix = "src",
    deps = [
        "//src/tsync-lib:tsync_if",
        "//src/tsync-utility-lib:tsync_utility_if",
        "//src/common",
    ],
    visibility = [
        ":__subpackages__",
    ],
)

cc_library(
    name = "tsync_daemon_lib",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
    ]),
    copts = ["-iquote src/tsync-daemon/src"],
    deps = [
        ":tsync_flatcfg",
        "//src/tsync-utility-lib:tsync_utility",
        "//src/flatcfg:flatcfg",
        "//src/common",
        "@score_baselibs//score/result:result",
    ],
    linkstatic = True,
    visibility = [
        "//tests/common/mocks:__subpackages__",
    ],
)

flatbuffer_cc_library(
    name = "tsync_flatcfg",
    srcs = ["src/tsync_flatcfg.fbs"],
)

genrule(
    name = "config_file",
    srcs = [
        "src/tsync_flatcfg.fbs",
        "src/tsync__Machine_flatcfg.json",
    ],
    outs = ["src/tsync__Machine_flatcfg.bin"],
    cmd = """
    $(location @flatbuffers//:flatc) --binary -o $(RULEDIR)/src/ $(SRCS)
    """,
    message = "Generating daemon_config binary",
    tools = ["@flatbuffers//:flatc"],
)

cc_binary(
    name = "tsync_daemon",
    deps = [
        ":tsync_daemon_lib",
    ],
    data = [
        ":config_file",
    ],
    visibility = ["//visibility:public"],
)
