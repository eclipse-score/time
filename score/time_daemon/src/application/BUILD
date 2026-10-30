# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

load("@score_baselibs//:bazel/unit_tests.bzl", "cc_unit_test_suites_for_host_and_qnx")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "timebase_handler",
    hdrs = [
        "timebase_handler.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/time_daemon:__pkg__",
        "//score/time_daemon/src/application:__subpackages__",
    ],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_library(
    name = "timebase_handler_factory",
    hdrs = ["timebase_handler_factory.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/time_daemon:__pkg__",
        "//score/time_daemon/src/application:__subpackages__",
    ],
    deps = [":timebase_handler"],
)

cc_library(
    name = "time_daemon_core",
    srcs = ["time_daemon.cpp"],
    hdrs = ["time_daemon.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon:__pkg__"],
    deps = [
        ":timebase_handler",
        ":timebase_handler_factory",
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:frontend",
        "@score_lifecycle//score/launch_manager:lifecycle_cc",
    ],
)

cc_library(
    name = "time_daemon_main",
    srcs = ["main.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon:__pkg__"],
    deps = [
        ":time_daemon_core",
        "@score_lifecycle//score/launch_manager:lifecycle_cc",
    ],
)

cc_binary(
    name = "time_daemon",
    srcs = ["timebase_handler_factory.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/time_daemon:__pkg__"],
    deps = [
        ":time_daemon_main",
        ":timebase_handler_factory",
        "//score/time_daemon/src/application/svt:svt_handler",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":time_daemon_test",
        ":timebase_handler_factory_test",
    ],
    test_suites_from_sub_packages = [
        "//score/time_daemon/src/application/generic:unit_test_suite",
        "//score/time_daemon/src/application/job_runner:unit_test_suite",
        "//score/time_daemon/src/application/svt:unit_test_suite",
    ],
    visibility = ["//score/time_daemon:__pkg__"],
)

cc_test(
    name = "time_daemon_test",
    srcs = ["time_daemon_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["unit"],
    deps = [
        ":time_daemon_core",
        ":timebase_handler",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@score_lifecycle//score/launch_manager:lifecycle_cc",
    ],
)

cc_test(
    name = "timebase_handler_factory_test",
    srcs = ["timebase_handler_factory_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["unit"],
    deps = [
        ":time_daemon_core",
        ":timebase_handler",
        ":timebase_handler_factory",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@score_lifecycle//score/launch_manager:lifecycle_cc",
    ],
)
