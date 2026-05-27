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
    name = "job_runner",
    srcs = ["job_runner.cpp"],
    hdrs = [
        "job_runner.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src:__subpackages__"],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":job_runner_test",
    ],
    visibility = ["//score/time_daemon/src/application:__pkg__"],
)

cc_test(
    name = "job_runner_test",
    srcs = [
        "job_runner_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":job_runner",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
