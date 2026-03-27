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
    name = "real_ptp_engine",
    srcs = [
        "real_ptp_engine.cpp",
    ],
    hdrs = [
        "real_ptp_engine.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/TimeDaemon:__subpackages__"],
    deps = [
        "//score/TimeDaemon/code/common:logging_contexts",
        "//score/TimeDaemon/code/common/data_types:ptp_time_info",
        "//score/libTSClient:gptp_ipc",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_test(
    name = "real_ptp_engine_test",
    srcs = ["real_ptp_engine_test.cpp"],
    tags = ["unit"],
    deps = [
        ":real_ptp_engine",
        "//score/libTSClient:gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":real_ptp_engine_test"],
    test_suites_from_sub_packages = [],
    visibility = ["//score/TimeDaemon:__subpackages__"],
)
