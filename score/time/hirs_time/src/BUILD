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
load("@score_baselibs//third_party/itf:py_unittest_qnx_test.bzl", "py_unittest_qnx_test")

cc_library(
    name = "hirs_clock",
    srcs = ["hirs_clock.cpp"],
    hdrs = [
        "hirs_clock.h",
        "hirs_clock_backend.h",
        "hirs_time.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "hirs_clock_mock",
    testonly = True,
    hdrs = ["hirs_clock_backend_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":hirs_clock",
        "//score/time/clock:clock_test_utils",
        "//score/time/hirs_time/src/details/stub_impl",
        "@googletest//:gtest",
    ],
)

# Production entry point: hirs_clock + production backend in a single dep.
# Production binaries dep on //score/time/hirs_time:hirs_time;
# test binaries dep on //score/time/hirs_time:hirs_time_mock.
cc_library(
    name = "hirs_clock_impl",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//visibility:public"],
    deps = select({
        "@platforms//os:qnx": ["//score/time/hirs_time/src/details/qtime:qclock"],
        "//conditions:default": ["//score/time/hirs_time/src/details/system_clock"],
    }) + [
        ":hirs_clock",
    ],
)

py_unittest_qnx_test(
    name = "qnx_unit_test_cases",
    test_suites = [
        "//score/time/hirs_time/src/details:qnx_unit_test_cases",
    ],
    visibility = ["//score/time/hirs_time:__pkg__"],
)

cc_test(
    name = "hirs_clock_test",
    srcs = ["hirs_clock_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":hirs_clock_mock",
        "//score/time/clock:clock_test_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [":hirs_clock_test"],
    test_suites_from_sub_packages = [
        "//score/time/hirs_time/src/details:unit_test_suite",
    ],
    visibility = ["//score/time/hirs_time:__pkg__"],
)
