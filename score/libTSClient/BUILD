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
    name = "gptp_ipc",
    srcs = [
        "gptp_ipc_publisher.cpp",
        "gptp_ipc_receiver.cpp",
    ],
    hdrs = [
        "gptp_ipc.h",
        "gptp_ipc_channel.h",
        "gptp_ipc_data.h",
        "gptp_ipc_publisher.h",
        "gptp_ipc_receiver.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    linkopts = select({
        "@platforms//os:qnx": [],
        "//conditions:default": ["-lrt"],
    }),
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [],
)

cc_test(
    name = "gptp_ipc_publisher_test",
    srcs = [
        "gptp_ipc_publisher_test.cpp",
        "gptp_ipc_test_utils.h",
    ],
    tags = ["unit"],
    deps = [
        ":gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "gptp_ipc_receiver_test",
    srcs = [
        "gptp_ipc_receiver_test.cpp",
        "gptp_ipc_test_utils.h",
    ],
    tags = ["unit"],
    deps = [
        ":gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "gptp_ipc_roundtrip_test",
    srcs = [
        "gptp_ipc_roundtrip_test.cpp",
        "gptp_ipc_test_utils.h",
    ],
    tags = ["unit"],
    deps = [
        ":gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":gptp_ipc_publisher_test",
        ":gptp_ipc_receiver_test",
        ":gptp_ipc_roundtrip_test",
    ],
    test_suites_from_sub_packages = [],
    visibility = ["//score:__subpackages__"],
)
