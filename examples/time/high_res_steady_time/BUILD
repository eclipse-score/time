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

# Run with: bazel run //examples/time/high_res_steady_time
alias(
    name = "high_res_steady_time",
    actual = "//examples/time/high_res_steady_time/src:high_res_steady_time",
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = ["//examples/time/high_res_steady_time/src:unit_test_suite"],
    visibility = ["//examples/time:__pkg__"],
)
