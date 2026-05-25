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

#alias to create svt publisher implementation objects
alias(
    name = "svt_publisher",
    actual = "//score/time_daemon/src/ipc/svt/publisher:factory",
    tags = ["QM"],
    visibility = [
        "//score/time_daemon:__subpackages__",
    ],
)

#alias to create svt receiver implementation objects
alias(
    name = "svt_receiver",
    actual = "//score/time_daemon/src/ipc/svt/receiver:factory",
    tags = ["QM"],
    visibility = [
        "//score/time/vehicle_time/details/td_impl:__pkg__",
        "//score/time_daemon:__subpackages__",
    ],
)

#alias to create svt receiver mock objects
alias(
    name = "svt_receiver_mock",
    actual = "//score/time_daemon/src/ipc/svt/receiver:factory_mock",
    tags = ["QM"],
    visibility = [
        "//score/time/vehicle_time/details/td_impl:__pkg__",
    ],
)

# receiver interface alias
cc_library(
    name = "receiver",
    srcs = [],
    hdrs = [
        "receiver.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon:__subpackages__"],
)

# receiver mock
cc_library(
    name = "receiver_mock",
    testonly = True,
    srcs = [],
    hdrs = [
        "receiver_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon:__subpackages__"],
    deps = [
        ":receiver",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "data_converter",
    srcs = [],
    hdrs = [
        "data_converter.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon:__subpackages__"],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = [
        "//score/time_daemon/src/ipc/core:unit_test_suite",
        "//score/time_daemon/src/ipc/svt:unit_test_suite",
    ],
    visibility = ["//score/time_daemon:__pkg__"],
)
