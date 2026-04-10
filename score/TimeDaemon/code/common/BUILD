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
    name = "logging_contexts",
    srcs = [],
    hdrs = [
        "logging_contexts.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score/TimeDaemon:__subpackages__"],
    deps = [],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = [
        "//score/TimeDaemon/code/common/data_types:unit_test_suite",
        "//score/TimeDaemon/code/common/data_flow:unit_test_suite",
        "//score/TimeDaemon/code/common/machines:unit_test_suite",
    ],
    visibility = ["//score/TimeDaemon:__pkg__"],
)
