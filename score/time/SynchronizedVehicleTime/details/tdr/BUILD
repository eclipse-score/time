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

[
    cc_library(
        name = name,
        testonly = testonly,
        srcs = [
            "factory_impl.cpp",
            "receiver.cpp",
            "receiver.h",  # placed into 'srcs' since this is an internal header
            "//score/time/SynchronizedVehicleTime/details:logging_contexts",  # placed into 'srcs' since this is an internal header
        ],
        hdrs = [
            "//score/time/SynchronizedVehicleTime/details:factory_impl_header",
        ],
        features = COMPILER_WARNING_FEATURES,
        visibility = [
            "//score/time/SynchronizedVehicleTime/details:__pkg__",
            "//score/time/SynchronizedVehicleTime/test:__subpackages__",
        ],
        deps = [
            "//score/time/SynchronizedVehicleTime:interfaces",
            "@score_baselibs//score/concurrency",
            "@score_baselibs//score/mw/log:frontend",
        ] + dependencies,
    )
    for name, testonly, dependencies in [
        (
            "tdr",
            False,
            [
                "//score/time/HighPrecisionLocalSteadyClock",
                "//score/time_daemon/src/ipc:svt_receiver",
            ],
        ),
        (
            "for_unit_testing",
            True,
            [
                "//score/time_daemon/src/ipc:svt_receiver_mock",
                "//score/time/HighPrecisionLocalSteadyClock:utest_mock",
            ],
        ),
    ]
]

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":factory_impl_test",
        ":receiver_integration_test",
        ":receiver_unit_test",
    ],
    visibility = ["//score/time/SynchronizedVehicleTime/details:__pkg__"],
)

cc_test(
    name = "factory_impl_test",
    srcs = [
        "factory_impl_test.cpp",
    ],
    tags = ["unit"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":for_unit_testing",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/safecpp/coverage_termination_handler",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_test(
    name = "receiver_integration_test",
    srcs = [
        "receiver_integration_test.cpp",
    ],
    tags = ["unit"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":tdr",
        "//score/time/HighPrecisionLocalSteadyClock:utest_mock",
        #integration like test that uses real svt_publisher instead of mock receiver
        "//score/time_daemon/src/ipc:svt_publisher",
        "@score_baselibs//score/language/safecpp/coverage_termination_handler",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "receiver_unit_test",
    srcs = [
        "receiver_unit_test.cpp",
    ],
    tags = ["unit"],
    visibility = ["//score/time:__subpackages__"],
    deps = [
        ":for_unit_testing",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/safecpp/coverage_termination_handler",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
