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

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "test_scenarios",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.hpp",
    ]),
    includes = ["src"],
    visibility = ["//score/time_daemon/tests/component_tests:__pkg__"],
    deps = [
        "//score/time_daemon/src/application/svt:svt_handler",
        "//score/time_daemon/src/ipc:svt_publisher",
        "//score/time_daemon/src/ipc:svt_receiver",
        "//score/time_daemon/src/ptp_machine/stub:gptp_stub_machine",
        "//score/time_daemon/src/verification_machine/svt:svt_verification_machine",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@score_test_scenarios//test_scenarios_cpp",
    ],
)
