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

# Public C++ API surface compatibility gate. See README.md in this package.
#
# Kept outside //score so that consumers testing @score_time//score/... do not
# pick up these Linux/clang-only checks.

load("@rules_multirun//:defs.bzl", "multirun")
load("@score_tooling//:defs.bzl", "api_surface_test")

# One entry per public (//visibility:public) C++ library alias of the module,
# plus the packages whose headers those aliases expose: {test name: target}.
# Not covered: binaries, the testonly *_mock aliases, and the <domain> production
# aliases, which add only backend implementations to <domain>:interface.
_API_SURFACES = {
    # Clock<Tag>, ClockSnapshot and statuses, exposed by every clock domain.
    "clock": "//score/time/clock:clock_core",
    # PTP data types, exposed by VehicleTime.
    "ptp_types": "//score/time/ptp:ptp_types",
    # Clock domains.
    "high_res_steady_time": "//score/time/high_res_steady_time:interface",
    "steady_time": "//score/time/steady_time:interface",
    "system_time": "//score/time/system_time:interface",
    "vehicle_time": "//score/time/vehicle_time:interface",
    # TimeDaemon libraries for custom daemons and timebase handlers.
    "time_daemon_core": "//score/time_daemon:time_daemon_core",
    "time_daemon_generic_timebase_handler": "//score/time_daemon:generic_timebase_handler",
    "time_daemon_gptp_shm_machine": "//score/time_daemon:gptp_shm_machine",
    "time_daemon_main": "//score/time_daemon:time_daemon_main",
    "time_daemon_msg_broker": "//score/time_daemon:msg_broker",
    "time_daemon_ptp_control_flow_divider": "//score/time_daemon:ptp_control_flow_divider",
    "time_daemon_ptp_machine": "//score/time_daemon:ptp_machine",
    "time_daemon_ptp_time_info": "//score/time_daemon:ptp_time_info",
    "time_daemon_svt_publisher": "//score/time_daemon:svt_publisher",
    "time_daemon_svt_verification_machine": "//score/time_daemon:svt_verification_machine",
    "time_daemon_timebase_handler_factory": "//score/time_daemon:timebase_handler_factory",
}

[
    api_surface_test(
        name = name,
        # Documentation enforcement is out of scope for the compatibility gate.
        check_docs = False,
        lock_file = name + ".lock.json",
        # Run only by name (//:api_surface_tests, CI api-surface job), not by //...
        tags = ["manual"],
        target = target,
    )
    for name, target in _API_SURFACES.items()
]

test_suite(
    name = "api_surface_tests",
    tests = [":" + name for name in _API_SURFACES],
    visibility = ["//:__pkg__"],
)

multirun(
    name = "update",
    commands = [":" + name + ".update" for name in _API_SURFACES],
    jobs = 0,
    tags = ["manual"],
    visibility = ["//:__pkg__"],
)
