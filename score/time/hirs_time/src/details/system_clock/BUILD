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

# Non-QNX production backend for HirsTime.
# Wraps std::chrono::high_resolution_clock and provides CreateBackend<HirsTime>().
cc_library(
    name = "system_clock",
    srcs = [
        "hirs_clock_backend_impl.cpp",
    ],
    hdrs = [
        "hirs_clock_backend_impl.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time/hirs_time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/hirs_time/src:hirs_clock",
    ],
)

cc_test(
    name = "hirs_clock_backend_impl_test",
    testonly = True,
    srcs = [
        "hirs_clock_backend_impl.cpp",
        "hirs_clock_backend_impl.h",
        "hirs_clock_backend_impl_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/hirs_time/src:hirs_clock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":hirs_clock_backend_impl_test",
    ],
    visibility = ["//score/time/hirs_time/src/details:__pkg__"],
)
