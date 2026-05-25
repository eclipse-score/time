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

# The public interface of the SteadyTime clock domain
alias(
    name = "steady_time",
    actual = ":steady_clock_impl",
    visibility = ["//visibility:public"],
)

# The public interface for test purposes (includes mock backend)
alias(
    name = "steady_time_mock",
    testonly = True,
    actual = ":steady_clock_mock",
    visibility = ["//visibility:public"],
)

# Interface without backend — for business-logic libraries that are tested
# via :steady_time_mock and deployed via :steady_time.
alias(
    name = "interface",
    actual = ":steady_clock",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "steady_clock_iface",
    hdrs = [
        "steady_clock_iface.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "steady_clock",
    srcs = ["steady_clock.cpp"],
    hdrs = ["steady_clock.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":steady_clock_iface",
        "//score/time/clock:clock_core",
    ],
)

cc_library(
    name = "steady_clock_mock",
    testonly = True,
    hdrs = ["steady_clock_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":steady_clock",
        ":steady_clock_iface",
        "//score/time/clock:clock_test_utils",
        "//score/time/steady_time/details/stub_impl",
        "@googletest//:gtest",
    ],
)

# Production entry point: steady_clock + production backend in a single dep.
# Production binaries dep on :steady_time; test binaries dep on :steady_time_mock.
cc_library(
    name = "steady_clock_impl",
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/time:__subpackages__",
    ],
    deps = [
        ":steady_clock",
        "//score/time/steady_time/details/steady_time_impl",
    ],
)

cc_test(
    name = "steady_clock_test",
    srcs = ["steady_clock_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":steady_clock_mock",
        "//score/time/clock:clock_test_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":steady_clock_test",
    ],
    test_suites_from_sub_packages = [
        "//score/time/steady_time/details:unit_test_suite",
    ],
    visibility = ["//score/time:__pkg__"],
)
