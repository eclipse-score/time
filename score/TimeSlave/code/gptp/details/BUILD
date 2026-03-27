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
    name = "ptp_types",
    hdrs = ["ptp_types.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "i_raw_socket",
    hdrs = ["i_raw_socket.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [],
)

cc_library(
    name = "i_network_identity",
    hdrs = ["i_network_identity.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [":ptp_types"],
)

cc_library(
    name = "raw_socket",
    srcs = select({
        "@platforms//os:qnx": ["//score/TimeSlave/code/gptp/platform/qnx:raw_socket_src"],
        "//conditions:default": ["//score/TimeSlave/code/gptp/platform/linux:raw_socket_src"],
    }),
    hdrs = ["raw_socket.h"],
    features = COMPILER_WARNING_FEATURES,
    linkopts = select({
        "@platforms//os:qnx": ["-lsocket"],
        "//conditions:default": [],
    }),
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":i_raw_socket",
        ":ptp_types",
    ],
)

cc_library(
    name = "frame_codec",
    srcs = ["frame_codec.cpp"],
    hdrs = ["frame_codec.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [":ptp_types"],
)

cc_library(
    name = "message_parser",
    srcs = ["message_parser.cpp"],
    hdrs = ["message_parser.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [":ptp_types"],
)

cc_library(
    name = "sync_state_machine",
    srcs = ["sync_state_machine.cpp"],
    hdrs = ["sync_state_machine.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":ptp_types",
        "//score/TimeDaemon/code/common/data_types:ptp_time_info",
    ],
)

cc_library(
    name = "pdelay_measurer",
    srcs = ["pdelay_measurer.cpp"],
    hdrs = ["pdelay_measurer.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":frame_codec",
        ":i_raw_socket",
        ":ptp_types",
        "//score/TimeDaemon/code/common/data_types:ptp_time_info",
    ],
)

cc_library(
    name = "network_identity",
    srcs = select({
        "@platforms//os:qnx": ["//score/TimeSlave/code/gptp/platform/qnx:network_identity_src"],
        "//conditions:default": ["//score/TimeSlave/code/gptp/platform/linux:network_identity_src"],
    }),
    hdrs = ["network_identity.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":i_network_identity",
        ":ptp_types",
    ],
)

cc_library(
    name = "gptp_details",
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":frame_codec",
        ":i_network_identity",
        ":i_raw_socket",
        ":message_parser",
        ":network_identity",
        ":pdelay_measurer",
        ":ptp_types",
        ":raw_socket",
        ":sync_state_machine",
    ],
)

cc_test(
    name = "pdelay_measurer_test",
    srcs = ["pdelay_measurer_test.cpp"],
    tags = ["unit"],
    deps = [
        ":pdelay_measurer",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "frame_codec_test",
    srcs = ["frame_codec_test.cpp"],
    tags = ["unit"],
    deps = [
        ":frame_codec",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "message_parser_test",
    srcs = ["message_parser_test.cpp"],
    tags = ["unit"],
    deps = [
        ":message_parser",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "sync_state_machine_test",
    srcs = ["sync_state_machine_test.cpp"],
    tags = ["unit"],
    deps = [
        ":sync_state_machine",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":frame_codec_test",
        ":message_parser_test",
        ":pdelay_measurer_test",
        ":sync_state_machine_test",
    ],
    test_suites_from_sub_packages = [],
    visibility = ["//score:__subpackages__"],
)
