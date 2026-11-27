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

"""Component requirements for start-up and shutdown of the time slave and time daemon."""

import time

from attribute_plugin import add_test_properties
from helpers import (
    output_of,
    present_shm_files,
    remove_shm_files,
    start_time_daemon,
    start_time_slave,
    wait_for,
    wait_until_synchronized,
)

# --- start-up -----------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__startup",
        "comp_req__time_slave__error_reporting",
    ],
    test_type="fault-injection",
    derivation_technique="error-guessing",
)
def test_time_slave_exits_when_interface_is_unavailable(target, platform):
    """An unrecoverable start-up error is logged and ends the time slave process."""
    remove_shm_files(target, platform)
    slave = start_time_slave(target, "doesnotexist0")
    try:
        wait_for(
            lambda: not slave.is_running(),
            10,
            "time_slave to exit",
            lambda: output_of(slave),
        )
        assert slave.get_exit_code() != 0
        output = output_of(slave)
        assert "GptpEngine initialization failed" in output
        assert "TimeSlave initialized" not in output
    finally:
        slave.stop()
        remove_shm_files(target, platform)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__startup",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_time_daemon_can_be_started_before_time_slave(
    target, platform, gptp_iface, grandmaster
):
    """The time daemon retries its initialization, so it picks up a time slave that starts later."""
    remove_shm_files(target, platform)
    daemon = start_time_daemon(target)
    try:
        time.sleep(3)
        assert daemon.is_running()
        assert "connected to IPC channel" not in output_of(daemon)

        slave = start_time_slave(target, gptp_iface)
        try:
            wait_for(
                lambda: "connected to IPC channel" in output_of(daemon),
                20,
                "time_daemon to finish its initialization",
                lambda: output_of(daemon),
            )
            wait_until_synchronized(target)
        finally:
            slave.stop()
    finally:
        daemon.stop()
        remove_shm_files(target, platform)


# --- shutdown -----------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_time_slave_releases_resources_on_shutdown(target, platform, time_stack):
    """On a stop signal the time slave stops, logs it, and removes its shared memory."""
    slave, _ = time_stack
    assert [f for f in present_shm_files(target, platform) if "gptp_" in f], (
        "time slave shared memory missing"
    )

    slave.stop()

    assert "TimeSlave stopped" in output_of(slave), (
        "time_slave did not shut down by itself"
    )
    assert not [f for f in present_shm_files(target, platform) if "gptp_" in f]


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__shutdown",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_time_daemon_stops_on_shutdown(time_stack):
    """On a stop signal the time daemon leaves its run loop and logs it."""
    _, daemon = time_stack
    wait_for(
        lambda: "connected to IPC channel" in output_of(daemon),
        20,
        "time_daemon to finish its initialization",
        lambda: output_of(daemon),
    )

    daemon.stop()

    assert "Run() finished" in output_of(daemon), (
        "time_daemon did not shut down by itself"
    )
