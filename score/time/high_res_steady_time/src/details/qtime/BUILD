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

load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")
load("@score_baselibs//third_party/itf:py_unittest_qnx_test.bzl", "py_unittest_qnx_test")

cc_library(
    name = "qclock",
    srcs = [
        "high_res_steady_qclock.cpp",
        "tick_provider.cpp",
        "tick_provider.h",
    ],
    hdrs = [
        "high_res_steady_qclock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    target_compatible_with = ["@platforms//os:qnx"],
    visibility = ["//score/time/high_res_steady_time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/high_res_steady_time/src:high_res_steady_clock",
        "@score_baselibs//score/os/qnx:neutrino",
    ],
)

cc_test(
    name = "high_res_steady_qclock_test",
    testonly = True,
    srcs = [
        "high_res_steady_qclock.cpp",
        "high_res_steady_qclock.h",
        "high_res_steady_qclock_test.cpp",
        "tick_provider.h",
        "tick_provider_mock.cpp",
        "tick_provider_mock.h",
    ],
    tags = ["unit"],
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/high_res_steady_time/src:high_res_steady_clock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/qnx:neutrino_qnx_mock",
    ],
)

cc_test(
    name = "tick_provider_test",
    testonly = True,
    srcs = [
        "tick_provider.cpp",
        "tick_provider.h",
        "tick_provider_test.cpp",
    ],
    tags = ["unit"],
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "factory_test",
    srcs = [
        "factory_test.cpp",
    ],
    tags = ["unit"],
    target_compatible_with = ["@platforms//os:qnx"],
    deps = [
        ":qclock",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

py_unittest_qnx_test(
    name = "qnx_unit_test_cases",
    test_cases = [
        ":factory_test",
        ":high_res_steady_qclock_test",
        ":tick_provider_test",
    ],
    visibility = ["//score/time/high_res_steady_time/src/details:__pkg__"],
)
