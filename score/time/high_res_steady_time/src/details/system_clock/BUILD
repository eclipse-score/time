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

# Non-QNX production backend for HighResSteadyTime.
# Wraps std::chrono::high_resolution_clock and provides CreateBackend<HighResSteadyTime>().
cc_library(
    name = "system_clock",
    srcs = [
        "high_res_steady_clock_backend_impl.cpp",
    ],
    hdrs = [
        "high_res_steady_clock_backend_impl.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time/high_res_steady_time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/high_res_steady_time/src:high_res_steady_clock",
    ],
)

cc_test(
    name = "high_res_steady_clock_backend_impl_test",
    testonly = True,
    srcs = [
        "high_res_steady_clock_backend_impl.cpp",
        "high_res_steady_clock_backend_impl.h",
        "high_res_steady_clock_backend_impl_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/high_res_steady_time/src:high_res_steady_clock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":high_res_steady_clock_backend_impl_test",
    ],
    visibility = ["//score/time/high_res_steady_time/src/details:__pkg__"],
)
