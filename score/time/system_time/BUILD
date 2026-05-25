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

# The public interface of the SystemTime clock domain
alias(
    name = "system_time",
    actual = ":system_clock_impl",
    visibility = ["//visibility:public"],
)

# The public interface for test purposes (includes mock backend)
alias(
    name = "system_time_mock",
    testonly = True,
    actual = ":system_clock_mock",
    visibility = ["//visibility:public"],
)

# Interface without backend — for business-logic libraries that are tested
# via :system_time_mock and deployed via :system_time.
alias(
    name = "interface",
    actual = ":system_clock",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "system_clock_iface",
    hdrs = [
        "system_clock_iface.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "system_clock",
    srcs = ["system_clock.cpp"],
    hdrs = ["system_clock.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":system_clock_iface",
        "//score/time/clock:clock_core",
    ],
)

cc_library(
    name = "system_clock_mock",
    testonly = True,
    hdrs = ["system_clock_mock.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":system_clock",
        ":system_clock_iface",
        "//score/time/clock:clock_test_utils",
        "//score/time/system_time/details/stub_impl",
        "@googletest//:gtest",
    ],
)

# Production entry point: system_clock + production backend in a single dep.
# Production binaries dep on :system_time; test binaries dep on :system_time_mock.
cc_library(
    name = "system_clock_impl",
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/time:__subpackages__",
    ],
    deps = [
        ":system_clock",
        "//score/time/system_time/details/system_time_impl",
    ],
)

cc_test(
    name = "system_clock_test",
    srcs = ["system_clock_test.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        ":system_clock_mock",
        "//score/time/clock:clock_test_utils",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":system_clock_test",
    ],
    test_suites_from_sub_packages = [
        "//score/time/system_time/details:unit_test_suite",
    ],
    visibility = ["//score/time:__pkg__"],
)
