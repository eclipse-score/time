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
    name = "time_handler",
    hdrs = ["vehicle_time_handler.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//examples/time/vehicle_time:__pkg__"],
    deps = [
        "//score/time/high_res_steady_time:interface",
        "//score/time/vehicle_time:interface",
    ],
)

cc_binary(
    name = "vehicle_time",
    srcs = ["main.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//examples/time/vehicle_time:__pkg__"],
    deps = [
        ":time_handler",
        "//score/time/high_res_steady_time",
        "//score/time/vehicle_time",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_test(
    name = "vehicle_time_handler_test",
    srcs = ["vehicle_time_handler_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = [
        "exclusive",
        "unit",
    ],
    deps = [
        ":time_handler",
        "//score/time/high_res_steady_time:high_res_steady_time_mock",
        "//score/time/vehicle_time:vehicle_time_mock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":vehicle_time_handler_test"],
    visibility = ["//examples/time/vehicle_time:__pkg__"],
)
