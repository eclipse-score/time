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

# Aggregates the unit_test_suite targets from all example time-domain applications.
#
# Run all example tests with:
#   bazel test //examples/time:unit_test_suite_host
#
# Or run everything — library, TimeDaemon, and examples — together:
#   bazel test //score/time:unit_test_suite_host
cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [],
    test_suites_from_sub_packages = [
        "//examples/time/vehicle_time:unit_test_suite",
        "//examples/time/high_res_steady_time:unit_test_suite",
        "//examples/time/steady_time:unit_test_suite",
        "//examples/time/system_time:unit_test_suite",
    ],
    visibility = ["//visibility:public"],
)
