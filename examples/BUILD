# examples/BUILD

# Load necessary Bazel rules
load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "get_current_time",
    srcs = ["get_current_time.cpp"],
    deps = [
        "//src/tsync-lib:tsync",
    ]
)
