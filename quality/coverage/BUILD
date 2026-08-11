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

load("//quality/coverage:coverage_scope.bzl", "coverage_scope")

# Defines the production source file scope for coverage reporting.
# The aspect traverses each dep's cc_library deps transitively —
# add new top-level production libraries here when they are not reachable
# via the deps of an existing entry.
coverage_scope(
    name = "time_coverage_scope",
    testonly = True,
    visibility = ["//quality/coverage:__subpackages__"],
    deps = [
        # score/time: package-level facades; transitively cover clock_core,
        # ptp_types, and all clock interfaces + implementations
        "//score/time/vehicle_time:vehicle_time",
        "//score/time/steady_time:steady_time",
        "//score/time/system_time:system_time",
        "//score/time/high_res_steady_time:high_res_steady_time",
        # score/time_daemon: binary alias transitively covers svt_handler,
        # job_runner, control_flow_divider, msg_broker, ptp_machine/shm,
        # verification_machine and their common dep chains, plus the binary's
        # own application sources
        "//score/time_daemon:time_daemon",
        # score/time_slave: binary alias covers application sources + gptp_engine
        "//score/time_slave:time_slave",
    ],
)
