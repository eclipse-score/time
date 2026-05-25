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

# The public interface of the HplsTime clock domain
alias(
    name = "hpls_time",
    actual = ":hpls_clock_impl",
    visibility = ["//visibility:public"],
)

# The public interface for test purposes (mock backend bundled)
alias(
    name = "hpls_time_mock",
    testonly = True,
    actual = ":hpls_clock_mock",
    visibility = ["//visibility:public"],
)

# Interface without backend — for business-logic libraries that are tested
# via :hpls_time_mock and deployed via :hpls_time.
alias(
    name = "interface",
    actual = ":hpls_clock",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "hpls_clock_iface",
    hdrs = [
        "hpls_clock_iface.h",
        "hpls_time.h",
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
    name = "hpls_clock",
    srcs = ["hpls_clock.cpp"],
    hdrs = ["hpls_clock.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":hpls_clock_iface",
        "//score/time/clock:clock_core",
    ],
)

cc_library(
    name = "hpls_clock_mock",
    testonly = True,
    hdrs = ["hpls_clock_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":hpls_clock",
        ":hpls_clock_iface",
        "//score/time/clock:clock_test_utils",
        "//score/time/hpls_time/details/stub_impl",
        "@googletest//:gtest",
    ],
)

# Production entry point: hpls_clock + production backend in a single dep.
# Production binaries dep on :hpls_time; test binaries dep on :hpls_time_mock.
cc_library(
    name = "hpls_clock_impl",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/time:__subpackages__",
    ],
    deps = select({
        "@platforms//os:qnx": ["//score/time/hpls_time/details/qtime:qclock"],
        "//conditions:default": ["//score/time/hpls_time/details/system_clock"],
    }) + [
        ":hpls_clock",
    ],
)

py_unittest_qnx_test(
    name = "qnx_unit_test_cases",
    test_suites = [
        "//score/time/hpls_time/details:qnx_unit_test_cases",
    ],
    visibility = ["//score/time:__pkg__"],
)

cc_test(
    name = "hpls_clock_test",
    srcs = ["hpls_clock_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":hpls_time_mock",
        "//score/time/clock:clock_test_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":hpls_clock_test",
    ],
    test_suites_from_sub_packages = [
        "//score/time/hpls_time/details:unit_test_suite",
    ],
    visibility = ["//score/time:__pkg__"],
)
