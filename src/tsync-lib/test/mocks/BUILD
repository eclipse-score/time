load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "mocks",
    hdrs = [
        "ConsumerTimeBaseValidationNotificationMock.h",
    ],
    strip_include_prefix = ".",
    deps = [
        "//src/tsync-lib:tsync_if",
    ],
    visibility = ["//visibility:public"]
)
