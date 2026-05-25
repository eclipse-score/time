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

# Production backend: reads live PTP data from the TimeDaemon via SvtReceiver.
# Link this target (or the forwarding alias //score/time/vehicle_time/details:td_impl)
# into the production binary to provide CreateBackend<VehicleTime>().
cc_library(
    name = "td_impl",
    srcs = [
        "vehicle_clock_impl.cpp",
        "//score/time/vehicle_time/details:logging_contexts",  # internal header
    ],
    hdrs = ["vehicle_clock_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time/vehicle_time:__pkg__"],
    deps = [
        "//score/TimeDaemon/code/ipc:svt_receiver",
        "//score/time/hpls_time:hpls_clock",
        "//score/time/vehicle_time:vehicle_clock",
        "//score/time/vehicle_time:vehicle_clock_iface",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_test(
    name = "vehicle_clock_impl_test",
    srcs = [
        "vehicle_clock_impl.cpp",
        "vehicle_clock_impl.h",
        "vehicle_clock_impl_test.cpp",
        "//score/time/vehicle_time/details:logging_contexts",  # internal header
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["unit"],
    deps = [
        "//score/TimeDaemon/code/ipc:svt_receiver_mock",
        "//score/time/hpls_time:hpls_time_mock",
        "//score/time/vehicle_time:vehicle_clock",
        "//score/time/vehicle_time:vehicle_clock_iface",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/safecpp/coverage_termination_handler",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":vehicle_clock_impl_test"],
    visibility = ["//score/time/vehicle_time:__pkg__"],
)
