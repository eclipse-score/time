# src/score/time/daemon/BUILD

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@flatbuffers//:build_defs.bzl", "flatbuffer_cc_library")

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
    srcs = ["src/score/time/daemon/tsync_flatcfg.fbs"],
)

genrule(
    name = "config_file",
    srcs = [
        "src/score/time/daemon/tsync_flatcfg.fbs",
        "src/score/time/daemon/tsync__Machine_flatcfg.json",
    ],
    outs = ["src/score/time/daemon/tsync__Machine_flatcfg.bin"],
    cmd = """
    $(location @flatbuffers//:flatc) --binary -o $(RULEDIR)/src/score/time/daemon/ $(SRCS)
    """,
    message = "Generating daemon_config binary",
    tools = ["@flatbuffers//:flatc"],
)


#exports_files(
#    [
#        "src/ConfigLoader.cpp",
#        "src/tsyncd_main.cpp",
#    ],
#    visibility = ["//tests/SCORE_UT:__pkg__"],
#)