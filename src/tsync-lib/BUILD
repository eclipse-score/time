# src/score/time/lib/BUILD
# include/BUILD

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
    name = "tsync",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h"
    ]),
    copts = ["-iquote src/tsync-lib/src"],
    hdrs = [
        ":tsync_if",
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
