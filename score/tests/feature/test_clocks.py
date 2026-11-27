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

"""Steady clock and high resolution steady clock."""

import json

import pytest
from attribute_plugin import add_test_properties
from helpers import (
    BIN,
    sample_vehicle_time,
)

# --- Steady Clock ------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__monotonic_clock_api",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_steady_clock_is_monotonic_and_matches_the_system_uptime(target):
    """SteadyClock advances like the monotonic clock of the target, which /proc/uptime counts as well."""
    exit_code, _ = target.execute("test -r /proc/uptime")
    if exit_code != 0:
        pytest.skip("no /proc/uptime to compare with")
    exit_code, output = target.execute(
        f"cat /proc/uptime; {BIN}/vehicle_time_probe --samples 20 --interval-ms 100; cat /proc/uptime"
    )
    assert exit_code == 0, output
    lines = output.decode().splitlines()
    uptime_before, uptime_after = (float(lines[i].split()[0]) for i in (0, -1))
    # Not before the first sample and not after the last, up to the resolution of /proc/uptime. Not the time the
    # target has been suspended, which is not counted by CLOCK_MONOTONIC.
    monotonic = [
        json.loads(line)["monotonic_time_ns"] / 1e9
        for line in lines
        if line.startswith("{")
    ]
    assert len(monotonic) == 20
    assert monotonic == sorted(monotonic) and monotonic[-1] > monotonic[0]
    assert uptime_before - 0.05 <= monotonic[0] and monotonic[-1] <= uptime_after + 0.05
    # 19 intervals of 100 ms. There is no upper bound: a loaded host may stretch the sleeps, and the span cannot exceed
    # the uptime bracket checked above.
    assert 1.8 <= monotonic[-1] - monotonic[0]


# --- High-Resolution Steady Clock --------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__high_res_clock_api",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_high_resolution_clock_has_nanosecond_resolution_and_advances(target):
    """The clock advances at the pace of the steady clock and is not rounded to microseconds or more."""
    samples = sample_vehicle_time(target, samples=20, interval_ms=50)
    clock = [s["steady_time_ns"] for s in samples]
    steady = [s["monotonic_time_ns"] for s in samples]

    assert len(clock) == 20 and all(b > a for a, b in zip(clock, clock[1:]))
    assert any(ns % 1_000 for ns in clock), "all values are multiples of 1 us"
    # The HighResSteadyClock of Linux is CLOCK_REALTIME-based, which the guest may slew a little.
    assert abs((clock[-1] - clock[0]) - (steady[-1] - steady[0])) < 100_000_000
