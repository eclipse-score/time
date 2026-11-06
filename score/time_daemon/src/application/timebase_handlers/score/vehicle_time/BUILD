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
load("//score/time_daemon/src/ipc/core:shared_memory_handler_copts.bzl", "SHARED_MEMORY_HANDLER_COPTS")

[
    cc_library(
        name = name,
        testonly = testonly,
        srcs = [
            "factory.cpp",
            "score_vehicle_timebase_handler.cpp",
        ],
        hdrs = [
            "factory.h",
            "score_vehicle_timebase_handler.h",
        ],
        copts = SHARED_MEMORY_HANDLER_COPTS,
        features = COMPILER_WARNING_FEATURES,
        visibility = ["//score/time_daemon:__subpackages__"],
        deps = [
            "//score/time_daemon/src/application/timebase_handlers:timebase_handler",
            "//score/time_daemon/src/application/timebase_handlers/generic:generic_timebase_handler",
            "//score/time_daemon/src/common/data_types:ptp_time_info",
            "//score/time_daemon/src/control_flow_divider/ptp:ptp_control_flow_divider",
            "//score/time_daemon/src/ipc/svt/publisher:factory",
            "//score/time_daemon/src/msg_broker",
        ] + deps,
    )
    for name, testonly, deps in [
        (
            "score_vehicle_timebase_handler",
            False,
            [
                "//score/time_daemon/src/ptp_machine/shm:gptp_shm_machine",
                "//score/time_daemon/src/verification_machine/svt:svt_verification_machine",
            ],
        ),
        (
            "score_vehicle_timebase_handler_for_utests",
            True,
            [
                "//score/time_daemon/src/ptp_machine/shm:gptp_shm_machine",
                "//score/time_daemon/src/verification_machine/svt:svt_verification_machine_for_utests",
            ],
        ),
    ]
]

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    cc_unit_tests = [
        ":score_vehicle_timebase_handler_integration_test",
    ],
    visibility = ["//score/time_daemon/src/application:__pkg__"],
)

cc_test(
    name = "score_vehicle_timebase_handler_integration_test",
    srcs = [
        "score_vehicle_timebase_handler_integration_test.cpp",
    ],
    tags = [
        "exclusive",
        "unit",
    ],
    deps = [
        ":score_vehicle_timebase_handler",
        "//score/time_daemon/src/ipc:svt_receiver",
        "//score/ts_client/src:gptp_ipc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
