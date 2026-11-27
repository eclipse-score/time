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

"""Fixtures of the end-to-end tests, made available by the conftest.py of the test directories."""

import time

import pytest
from gptp_grandmaster import (
    GrandMaster,
)
from helpers import (
    GM_IFACE,
    GM_OFFSET_NS,
    HOST_CLOCK_MAX_SKEW,
    JUMP_DEBOUNCE_S,
    PLATFORMS,
    host_clock_skew,
    probing,
    running_time_stack,
    wait_for_stream_reliable,
)

SKIP_HOST_CLOCK_CHECK = "--skip-host-clock-check"


def add_host_clock_option(parser):
    """Registers the option that disables the host_clock_check fixture, called by the pytest_addoption of each conftest.py."""
    parser.addoption(
        SKIP_HOST_CLOCK_CHECK,
        action="store_true",
        default=False,
        help="run even if the clock of the host is not running at the pace of the hardware clock",
    )


@pytest.fixture(scope="session", autouse=True)
def host_clock_check(request):
    """Stops the session if the clocks of the host run at another pace than the hardware clock.

    The grandmaster takes its time from the host's CLOCK_REALTIME and the guest's clock follows the hardware clock,
    so a host clock that is slewed by a few percent makes every test that compares the two fail on its drift
    check. Without this check these failures show up as unrelated assertions in many tests.
    """
    if request.config.getoption(SKIP_HOST_CLOCK_CHECK, default=False):
        return
    skew = host_clock_skew()
    if skew is None or abs(skew) <= HOST_CLOCK_MAX_SKEW:
        return
    pytest.exit(
        f"The clock of the host is not running at the pace of the hardware clock: CLOCK_MONOTONIC advanced "
        f"{skew:+.2%} against CLOCK_MONOTONIC_RAW (limit ±{HOST_CLOCK_MAX_SKEW:.1%}).\n"
        "The grandmaster uses the host's CLOCK_REALTIME and the guest follows the hardware clock, so the "
        "time tests would see a rate error of this size and fail their drift checks.\n"
        "Something is adjusting the tick or the frequency of the host clock, for example an NTP client in a "
        "WSL2 distro (see 'Host Clock' in docs/module/manuals/integration_tests.rst).\n"
        f"Pass {SKIP_HOST_CLOCK_CHECK} to pytest (with Bazel: --test_arg={SKIP_HOST_CLOCK_CHECK}) to run anyway.",
        returncode=1,
    )


@pytest.fixture
def platform(target):
    exit_code, output = target.execute("uname -s")
    assert exit_code == 0, output
    return PLATFORMS[output.decode().strip()]


@pytest.fixture
def gptp_iface(target, platform):
    """Name of the network interface of the target that is connected to the grandmaster."""
    exit_code, output = target.execute(platform.gptp_iface_command)
    assert exit_code == 0 and output.strip(), output
    return output.decode().strip()


@pytest.fixture
def time_stack(target, platform, gptp_iface):
    with running_time_stack(target, platform, gptp_iface) as stack:
        yield stack


@pytest.fixture
def probe_stream(target, time_stack):
    # Depends on time_stack so that the probe never starts before the time stack's shared memory exists.
    with probing(target) as stream:
        yield stream


@pytest.fixture
def event_probe_stream(target, time_stack):
    """Probe stream whose probe subscribed to the notifications of the vehicle clock."""
    with probing(target, events=True) as stream:
        yield stream


@pytest.fixture
def grandmaster():
    """gPTP grandmaster on GM_IFACE, offset by GM_OFFSET_NS from the host clock."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        yield master


@pytest.fixture
def jump_detection_armed(time_stack, probe_stream, grandmaster):
    """Synchronized time stack in which the start-up suppression of time jumps (5 s) is over."""
    wait_for_stream_reliable(probe_stream)
    time.sleep(JUMP_DEBOUNCE_S + 2)
    assert probe_stream.latest()["reliable"]
    return probe_stream
