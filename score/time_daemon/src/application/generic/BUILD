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
    name = "generic_timebase_handler",
    hdrs = [
        "generic_timebase_handler.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/application:__subpackages__"],
    deps = [
        "//score/time_daemon/src/application:timebase_handler",
        "//score/time_daemon/src/application/job_runner",
        "//score/time_daemon/src/common:logging_contexts",
        "//score/time_daemon/src/common/data_flow",
        "//score/time_daemon/src/common/machines",
        "//score/time_daemon/src/msg_broker",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":generic_timebase_handler_test",
    ],
    visibility = ["//score/time_daemon/src/application:__pkg__"],
)

cc_test(
    name = "generic_timebase_handler_test",
    srcs = [
        "generic_timebase_handler_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":generic_timebase_handler",
        "//score/time_daemon/src/common/data_flow",
        "//score/time_daemon/src/common/machines",
        "//score/time_daemon/src/msg_broker",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
