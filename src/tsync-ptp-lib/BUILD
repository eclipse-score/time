load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tsync_ptp",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
    ]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "//src/common",
        "//src/tsync-utility-lib:tsync_utility",
    ],
    visibility = ["//visibility:public"],
)
