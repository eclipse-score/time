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

# PTP-protocol notification data types, reusable across all PTP time domains.
cc_library(
    name = "ptp_types",
    hdrs = [
        "local_ptp_device_timer.h",
        "master_ptp_device_timer.h",
        "pdelay_measurement_data.h",
        "port_identity.h",
        "time_slave_sync_data.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/time:__subpackages__"],
)

cc_test(
    name = "time_slave_sync_data_test",
    srcs = ["time_slave_sync_data_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["unit"],
    deps = [
        ":ptp_types",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "pdelay_measurement_data_test",
    srcs = ["pdelay_measurement_data_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["unit"],
    deps = [
        ":ptp_types",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":time_slave_sync_data_test",
        ":pdelay_measurement_data_test",
    ],
    visibility = ["//score/time/ptp:__pkg__"],
)
