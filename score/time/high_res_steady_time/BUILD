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

# The public interface of the HighResSteadyTime clock domain
alias(
    name = "high_res_steady_time",
    actual = "//score/time/high_res_steady_time/src:high_res_steady_clock_impl",
    visibility = ["//visibility:public"],
)

# The public interface for test purposes (mock backend bundled)
alias(
    name = "high_res_steady_time_mock",
    testonly = True,
    actual = "//score/time/high_res_steady_time/src:high_res_steady_clock_mock",
    visibility = ["//visibility:public"],
)

# Interface without backend — for business-logic libraries that are tested
# via :high_res_steady_time_mock and deployed via :high_res_steady_time.
alias(
    name = "interface",
    actual = "//score/time/high_res_steady_time/src:high_res_steady_clock",
    visibility = ["//visibility:public"],
)

py_unittest_qnx_test(
    name = "qnx_unit_test_cases",
    test_suites = ["//score/time/high_res_steady_time/src:qnx_unit_test_cases"],
    visibility = ["//score/time:__pkg__"],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = ["//score/time/high_res_steady_time/src:unit_test_suite"],
    visibility = ["//score/time:__pkg__"],
)
