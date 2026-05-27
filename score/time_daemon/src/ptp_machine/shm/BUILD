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
    name = "gptp_shm_machine",
    srcs = [
        "factory.cpp",
    ],
    hdrs = [
        "factory.h",
        "gptp_shm_machine.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/time_daemon:__subpackages__"],
    deps = [
        "//score/libTSClient:gptp_ipc",
        "//score/time_daemon/src/ptp_machine/core:ptp_machine",
        "//score/time_daemon/src/ptp_machine/shm/details:shm_ptp_engine",
    ],
)

cc_test(
    name = "gptp_shm_machine_test",
    srcs = ["gptp_shm_machine_test.cpp"],
    tags = ["unit"],
    deps = [
        ":gptp_shm_machine",
        "//score/libTSClient:gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":gptp_shm_machine_test"],
    test_suites_from_sub_packages = [
        "//score/time_daemon/src/ptp_machine/shm/details:unit_test_suite",
    ],
    visibility = ["//score/time_daemon:__subpackages__"],
)
