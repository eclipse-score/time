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
    name = "machines",
    srcs = [
        "base_machine.cpp",
        "event_driven_machine.cpp",
        "periodic_machine.cpp",
    ],
    hdrs = [
        "base_machine.h",
        "event_driven_machine.h",
        "periodic_machine.h",
        "proactive_machine.h",
        "reactive_machine.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/time_daemon:__subpackages__"],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":machines_test",
    ],
    visibility = ["//score/time_daemon:__pkg__"],
)

cc_test(
    name = "machines_test",
    srcs = [
        "base_machine_test.cpp",
        "event_driven_machine_test.cpp",
        "periodic_machine_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":machines",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
