load("@rules_cc//cc:defs.bzl", "cc_library")
# load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "test_matchers",
    hdrs = [
        "matcher_operators.h",
    ],
    strip_include_prefix = ".",
    deps = [
        "//src/tsync-utility-lib:tsync_utility_if",
        "@score_baselibs//score/language/futurecpp",
    ],
    visibility = ["//visibility:public"]
)
