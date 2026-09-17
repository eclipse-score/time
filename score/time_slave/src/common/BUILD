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

cc_library(
    name = "logging_contexts",
    hdrs = ["logging_contexts.h"],
    visibility = ["//score/time_slave:__subpackages__"],
)

cc_library(
    name = "definitions",
    hdrs = ["definitions.h"],
    visibility = ["//score/time_slave:__subpackages__"],
)

cc_test(
    name = "definitions_test",
    srcs = ["definitions_test.cpp"],
    tags = ["unit"],
    deps = [
        ":definitions",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":definitions_test"],
    visibility = ["//score:__subpackages__"],
)
