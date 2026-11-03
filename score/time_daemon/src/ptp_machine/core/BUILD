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
    name = "ptp_machine",
    srcs = [
    ],
    hdrs = [
        "ptp_machine.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/time_daemon:__subpackages__"],
    deps = [
        "//score/time/high_res_steady_time:interface",
        "//score/time_daemon/src/common/data_flow",
        "//score/time_daemon/src/common/machines",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":ptp_machine_test",
    ],
    test_suites_from_sub_packages = [
    ],
    visibility = ["//score/time_daemon:__subpackages__"],
)

cc_test(
    name = "ptp_machine_test",
    srcs = [
        "ptp_engine_mock.h",
        "ptp_machine_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":ptp_machine",
        "//score/time_daemon/src/common/data_types:ptp_time_info",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
