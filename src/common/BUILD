# src/score/time/lib/BUILD
# include/BUILD

load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "common_if",
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "common",
    linkstatic = True,
    hdrs = [
        ":common_if",
    ],
    srcs = glob([
        "src/**/*.cpp",
    ]),
    copts = ["-iquote src/common/src"],
    deps = [
        ":common_if",
    ],
    visibility = ["//visibility:public"],
)
