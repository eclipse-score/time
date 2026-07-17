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
    copts = ["-g"],
    includes = ["src"],
    visibility = ["//score/time_slave/tests/integration_tests:__pkg__"],
    deps = [
        "//score/time_slave/src/gptp:gptp_engine",
        "//score/time_slave/src/gptp/details:network_identity",
        "//score/time_slave/src/gptp/details:ptp_types",
        "//score/time_slave/src/gptp/details:raw_socket",
        "@score_baselibs//score/mw/log:console_only_backend",
        "@score_test_scenarios//test_scenarios_cpp",
    ],
)
