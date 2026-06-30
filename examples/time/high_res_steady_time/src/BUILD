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

# Library containing only the testable business-logic — no main(), no signal handling.
cc_library(
    name = "time_handler",
    hdrs = ["high_res_steady_time_handler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//examples/time/high_res_steady_time:__pkg__"],
    deps = [
        "//score/time/high_res_steady_time:interface",
    ],
)

# Example binary: reads HighResSteadyTime once per second and prints the snapshot to stdout.
cc_binary(
    name = "high_res_steady_time",
    srcs = ["main.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//examples/time/high_res_steady_time:__pkg__"],
    deps = [
        ":time_handler",
        "//score/time/high_res_steady_time",
    ],
)

cc_test(
    name = "high_res_steady_time_handler_test",
    srcs = ["high_res_steady_time_handler_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = [
        "exclusive",
        "unit",
    ],
    deps = [
        ":time_handler",
        "//score/time/high_res_steady_time:high_res_steady_time_mock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":high_res_steady_time_handler_test"],
    visibility = ["//examples/time/high_res_steady_time:__pkg__"],
)
