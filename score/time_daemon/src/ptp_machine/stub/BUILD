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
            "factory.cpp",
        ],
        hdrs = [
            "factory.h",
            "gptp_stub_machine.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        includes = [],
        tags = ["QM"],
        visibility = ["//score/time_daemon:__subpackages__"],
        deps = [
            "//score/time_daemon/src/common:logging_contexts",
            "//score/time_daemon/src/common/machines",
            "//score/time_daemon/src/ptp_machine/core:ptp_machine",
            "//score/time_daemon/src/ptp_machine/stub/details:stub_ptp_engine",
            "@score_baselibs//score/mw/log:frontend",
        ] + deps,
    )
    for name, testonly, deps in [
        (
            "gptp_stub_machine",
            False,
            [
                "//score/time/high_res_steady_time:high_res_steady_time",
            ],
        ),
        (
            "gptp_stub_machine_for_utests",
            True,
            [
                "//score/time/high_res_steady_time:high_res_steady_time_mock",
            ],
        ),
    ]
]

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":gptp_stub_machine_test",
    ],
    test_suites_from_sub_packages = [
    ],
    visibility = ["//score/time_daemon:__subpackages__"],
)

cc_test(
    name = "gptp_stub_machine_test",
    srcs = [
        "gptp_stub_machine_integration_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":gptp_stub_machine_for_utests",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)
