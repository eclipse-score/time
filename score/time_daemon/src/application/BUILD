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
    name = "timebase_handler",
    hdrs = [
        "timebase_handler.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/application:__subpackages__"],
    deps = [
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_binary(
    name = "time_daemon",
    srcs = [
        "main.cpp",
        "time_daemon.cpp",
        "time_daemon.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/time_daemon:__pkg__"],
    deps = [
        "//score/time_daemon/src/application/svt:svt_handler",
        "//score/time_daemon/src/common:logging_contexts",
        "@score_baselibs//score/concurrency",
        "@score_lifecycle_health//src/lifecycle_client_lib",
        # "@score_logging//score/mw/log",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = [
        "//score/time_daemon/src/application/job_runner:unit_test_suite",
        "//score/time_daemon/src/application/svt:unit_test_suite",
    ],
    visibility = ["//score/time_daemon:__pkg__"],
)
