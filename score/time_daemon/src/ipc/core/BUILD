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
    name = "shared_memory_handler",
    srcs = [],
    hdrs = [
        "shared_memory_handler.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/ipc:__subpackages__"],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/memory/shared",
        "@score_baselibs//score/memory/shared/typedshm/typedshm_wrapper:typedmemory",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_library(
    name = "publisher_impl",
    srcs = [],
    hdrs = [
        "publisher_impl.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/ipc:__subpackages__"],
    deps = [
        ":shared_memory_handler",
        "//score/time_daemon/src/common/data_flow",
        "//score/time_daemon/src/common/machines",
        "//score/time_daemon/src/ipc:data_converter",
    ],
)

cc_library(
    name = "receiver_impl",
    srcs = [],
    hdrs = [
        "receiver_impl.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/ipc:__subpackages__"],
    deps = [
        ":shared_memory_handler",
        "//score/time_daemon/src/ipc:receiver",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":shared_memory_handler_test",
        ":publisher_impl_test",
        ":receiver_impl_test",
    ],
    visibility = ["//score/time_daemon:__subpackages__"],
)

cc_test(
    name = "publisher_impl_test",
    srcs = [
        "publisher_impl_test.cpp",
        "test_types.h",
    ],
    tags = ["unit"],
    deps = [
        ":publisher_impl",
        "//score/time_daemon/src/ipc:data_converter",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_test(
    name = "receiver_impl_test",
    srcs = [
        "receiver_impl_test.cpp",
        "test_types.h",
    ],
    tags = ["unit"],
    deps = [
        ":receiver_impl",
        "//score/time_daemon/src/ipc:data_converter",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_test(
    name = "shared_memory_handler_test",
    srcs = [
        "shared_memory_handler_test.cpp",
        "test_types.h",
    ],
    tags = ["unit"],
    deps = [
        ":shared_memory_handler",
        "//score/time_daemon/src/common/data_types:ptp_time_info",
        "//score/time_daemon/src/ipc:data_converter",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
