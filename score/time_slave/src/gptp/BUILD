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
    name = "gptp_engine",
    srcs = ["gptp_engine.cpp"],
    hdrs = ["gptp_engine.h"],
    features = COMPILER_WARNING_FEATURES,
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsocket",
            "-lc",
        ],
        "//conditions:default": ["-lpthread"],
    }),
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/time_slave/src/common:logging_contexts",
        "//score/time_slave/src/gptp/details:gptp_details",
        "//score/time_slave/src/gptp/phc:phc_adjuster",
        "//score/ts_client/src:gptp_ipc",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_test(
    name = "gptp_engine_test",
    srcs = ["gptp_engine_test.cpp"],
    tags = ["unit"],
    deps = [
        ":gptp_engine",
        "//score/time_slave/src/gptp/details:network_identity",
        "//score/time_slave/src/gptp/details:raw_socket",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":gptp_engine_test"],
    test_suites_from_sub_packages = [
        "//score/time_slave/src/gptp/details:unit_test_suite",
        "//score/time_slave/src/gptp/instrument:unit_test_suite",
        "//score/time_slave/src/gptp/record:unit_test_suite",
    ],
    visibility = ["//score:__subpackages__"],
)
