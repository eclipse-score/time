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
    name = "msg_broker",
    srcs = [
        "topic.cpp",
    ],
    hdrs = [
        "msg_broker.h",
        "subscription.h",
        "topic.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src:__subpackages__"],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "//score/time_daemon/src/common/data_flow",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":msg_broker_test",
    ],
    visibility = ["//score/time_daemon:__pkg__"],
)

cc_test(
    name = "msg_broker_test",
    srcs = [
        "msg_broker_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":msg_broker",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
