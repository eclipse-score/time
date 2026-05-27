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

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_test_suite",
    test_suites_from_sub_packages = [
        "//score/time_daemon/src/application:unit_test_suite",
        "//score/time_daemon/src/common:unit_test_suite",
        "//score/time_daemon/src/control_flow_divider:unit_test_suite",
        "//score/time_daemon/src/ipc:unit_test_suite",
        "//score/time_daemon/src/msg_broker:unit_test_suite",
        "//score/time_daemon/src/ptp_machine:unit_test_suite",
        "//score/time_daemon/src/verification_machine:unit_test_suite",
    ],
    visibility = ["//score:__pkg__"],
)
