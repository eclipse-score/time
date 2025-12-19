load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "flatcfg",
    srcs = glob([
        "src/**/*.cpp", # Ensure these are correct; consider 'src/*.cpp' if all are in 'src/'
        "src/**/*.h",
    ]),
    hdrs = glob([
        "include/**/*.h", # Glob for other headers in current package/subdirs
    ]),
    strip_include_prefix = "include",
    deps = [
        "//src/common",
        "//src/flatbuffer:flatbuffer",
        "@flatbuffers//:flatbuffers",
        #"@score_baselibs//score/mw/log",
        "@score_baselibs//score/result",
    ],
    copts = ["-iquote src/flatcfg/src"],
    visibility = ["//visibility:public"],
)
