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
    name = "ptp_time_info",
    srcs = ["ptp_time_info.cpp"],
    hdrs = [
        "ptp_time_info.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/TimeDaemon:__subpackages__"],
    deps = ["//score/time/HighPrecisionLocalSteadyClock:interface"],
)

cc_test(
    name = "ptp_time_info_test",
    srcs = [
        "ptp_time_info_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":ptp_time_info",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":ptp_time_info_test",
    ],
    visibility = ["//score/TimeDaemon/code/common:__pkg__"],
)
