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
load("@score_baselibs//third_party/itf:py_unittest_qnx_test.bzl", "py_unittest_qnx_test")

py_unittest_qnx_test(
    name = "qnx_unit_test_cases",
    test_suites = [
        "//score/time/high_res_steady_time/src/details/qtime:qnx_unit_test_cases",
        "//score/time/high_res_steady_time/src/details/system_clock:qnx_unit_test_cases",
    ],
    visibility = ["//score/time/high_res_steady_time:__subpackages__"],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [],
    test_suites_from_sub_packages = [
        "//score/time/high_res_steady_time/src/details/system_clock:unit_test_suite",
    ],
    visibility = ["//score/time/high_res_steady_time:__subpackages__"],
)
