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

filegroup(
    name = "raw_socket_src",
    srcs = [
        "qnx_raw_shim.cpp",
        "raw_socket.cpp",
    ],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "timestamp_select",
    hdrs = ["timestamp_select.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
)

cc_test(
    name = "timestamp_select_test",
    srcs = ["timestamp_select_test.cpp"],
    tags = ["unit"],
    deps = [
        ":timestamp_select",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":timestamp_select_test"],
    visibility = ["//score:__subpackages__"],
)

filegroup(
    name = "network_identity_src",
    srcs = ["network_identity.cpp"],
    visibility = ["//score:__subpackages__"],
)

filegroup(
    name = "phc_adjuster_src",
    srcs = ["phc_adjuster.cpp"],
    visibility = ["//score:__subpackages__"],
)
