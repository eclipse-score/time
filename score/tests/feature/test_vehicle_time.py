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

"""Value, pace, precision and consistency of the vehicle time."""

import statistics
import time

import pytest
from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    GrandMaster,
)
from helpers import (
    GM_IFACE,
    GM_OFFSET_NS,
    SYNC_TIMEOUT_S,
    SYNC_TOLERANCE_NS,
    TIMEOUT_S,
    assert_following_grandmaster,
    drifts_from_grandmaster_pace_ns,
    probing,
    sample_vehicle_time,
    vehicle_time_jumps_ns,
    wait_for_stream,
    wait_for_stream_in_sync,
    wait_for_stream_reliable,
    wait_until_synchronized,
)
from known_issues import (
    XFAIL_257_NO_SOFTWARE_TIMESTAMPS,
    XFAIL_310_VEHICLE_TIME_MIXES_CLOCKS,
    XFAIL_342_RATE_DEVIATION_NOT_APPLIED,
    XFAIL_342_RATE_DEVIATION_NOT_APPLIED_PRECISION,
)

# --- value range of vehicle time --------------------------------------------------------------------------------

# Grandmaster times (in s) at the ends of the range of vehicle time. The upper one is far enough from 2^32 s that the
# test does not wrap in the time it takes.
RANGE_LIMITS = [
    pytest.param(10, id="start_of_range"),
    pytest.param(2**32 - 600, id="end_of_range"),
]


def grandmaster_offset_for_time_s(seconds):
    return seconds * 1_000_000_000 - time.time_ns()


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_tp_format",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@pytest.mark.parametrize("grandmaster_time_s", RANGE_LIMITS)
def test_synchronizes_at_the_ends_of_the_time_range(
    time_stack, probe_stream, grandmaster_time_s
):
    """A grandmaster time close to 0 s or to 2^32 s is followed like any other."""
    with GrandMaster(
        GM_IFACE, offset_ns=grandmaster_offset_for_time_s(grandmaster_time_s)
    ):
        wait_for_stream_reliable(probe_stream)
        mark = len(probe_stream.samples())
        time.sleep(3)
        samples = probe_stream.samples()[mark:]

    assert_following_grandmaster(samples)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_tp_format",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_310_VEHICLE_TIME_MIXES_CLOCKS
@pytest.mark.parametrize("grandmaster_time_s", RANGE_LIMITS)
def test_vehicle_time_matches_grandmaster_at_the_ends_of_the_time_range(
    target, time_stack, grandmaster_time_s
):
    """Vehicle time has the time scale of the grandmaster, from 0 s to less than 2^32 s."""
    with GrandMaster(
        GM_IFACE, offset_ns=grandmaster_offset_for_time_s(grandmaster_time_s)
    ) as master:
        wait_until_synchronized(target)

        before = master.time_ns()
        (sample,) = sample_vehicle_time(target)
        after = master.time_ns()

    assert (
        before - SYNC_TOLERANCE_NS
        <= sample["vehicle_time_ns"]
        <= after + SYNC_TOLERANCE_NS
    )
    assert 0 <= sample["vehicle_time_ns"] < 2**32 * 1_000_000_000


# --- rate of the grandmaster ------------------------------------------------------------------------------------

# The grandmaster runs 5 % fast: well above the rate error of the guest clock and the noise of the measurement.
FAST_GRANDMASTER_PPM = 50_000

FAST_GRANDMASTER_RATE = 1 + FAST_GRANDMASTER_PPM / 1_000_000


def mean_rate_deviation(samples):
    return statistics.mean(s["rate_deviation"] for s in samples)


def wait_for_rate_deviation_below(stream, limit):
    wait_for_stream(
        stream,
        lambda s: (
            s["rate_deviation"] > 0
            and mean_rate_deviation(stream.samples()[-10:]) < limit
        ),
        SYNC_TIMEOUT_S,
        f"rate deviation below {limit}",
    )


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_rate_dev",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
def test_rate_deviation_indicates_the_pace_of_the_grandmaster(
    time_stack, probe_stream, grandmaster
):
    """The rate deviation (local time per grandmaster time) drops by the 5 % that the grandmaster gets faster."""
    wait_for_stream_reliable(probe_stream)
    time.sleep(1)
    before = mean_rate_deviation(probe_stream.samples()[-20:])

    grandmaster.set_rate_ppm(FAST_GRANDMASTER_PPM)
    wait_for_rate_deviation_below(probe_stream, before - 0.02)
    time.sleep(1)
    after = mean_rate_deviation(probe_stream.samples()[-20:])

    assert before / after == pytest.approx(FAST_GRANDMASTER_RATE, abs=0.01)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_rate_comp",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_342_RATE_DEVIATION_NOT_APPLIED
def test_vehicle_time_between_updates_is_compensated_for_the_rate_deviation(
    time_stack, probe_stream, grandmaster
):
    """Without sync messages, vehicle time keeps advancing at the pace of the grandmaster, not of the local clock."""
    wait_for_stream_reliable(probe_stream)
    grandmaster.set_rate_ppm(FAST_GRANDMASTER_PPM)
    wait_for_rate_deviation_below(probe_stream, 0.98)

    # Shorter than the timeout (3.3 s), after which the time would be reported as not valid.
    grandmaster.pause_sync()
    mark = len(probe_stream.samples())
    time.sleep(2)
    samples = probe_stream.samples()[mark:]
    grandmaster.resume_sync()

    assert len(samples) > 10
    first, last = samples[0], samples[-1]
    # Vehicle time elapsed per local time elapsed: 1 / rate deviation (about 1.03 - 1.05) if compensated, 1 if not.
    # Local time of the SteadyClock (CLOCK_MONOTONIC), against which the rate deviation is determined.
    pace = (last["vehicle_time_ns"] - first["vehicle_time_ns"]) / (
        last["monotonic_time_ns"] - first["monotonic_time_ns"]
    )
    assert pace == pytest.approx(1 / mean_rate_deviation(samples), abs=0.01)
    assert pace > 1.015


# --- reading vehicle time without blocking -----------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__vehicle_time_ctrl_flow",
    ],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
@pytest.mark.parametrize("how", ["stopped", "killed"])
def test_vehicle_time_can_be_read_while_the_time_daemon_does_not_run(
    target, time_stack, probe_stream, grandmaster, how
):
    """An application that reads vehicle time is not blocked when the process that writes it is stopped or gone."""
    _, daemon = time_stack
    wait_for_stream_reliable(probe_stream)

    target.execute(f"kill -{'STOP' if how == 'stopped' else 'KILL'} {daemon.pid()}")
    try:
        mark = len(probe_stream.samples())
        time.sleep(3)
        samples = probe_stream.samples()[mark:]
    finally:
        if how == "stopped":
            target.execute(f"kill -CONT {daemon.pid()}")

    # One sample is due every 20 ms; most of them have to be there.
    assert len(samples) > 60, f"only {len(samples)} samples in 3 s"
    vehicle = [s["vehicle_time_ns"] for s in samples]
    assert vehicle == sorted(vehicle) and vehicle[-1] > vehicle[0], (
        "vehicle time does not advance"
    )


# --- rate deviation before the first measurement -----------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_rate_dev_init",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_no_rate_deviation_is_indicated_before_the_first_measurement(
    time_stack, probe_stream
):
    """Without a grandmaster, no rate deviation is measured: 0 is the value that tells so, as in the other tests."""
    time.sleep(2)
    samples = probe_stream.samples()
    assert len(samples) > 10
    assert [s["rate_deviation"] for s in samples if s["rate_deviation"] != 0] == []

    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS):
        wait_for_stream(
            probe_stream,
            lambda s: s["rate_deviation"] > 0,
            SYNC_TIMEOUT_S,
            "a rate deviation to be measured",
        )
        # A local clock that is off by more than 20 % is not a rate deviation but a broken measurement.
        assert 0.8 < probe_stream.latest()["rate_deviation"] < 1.2, (
            probe_stream.latest()
        )


# --- updates are applied as they are -----------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_apply_update",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_vehicle_time_follows_a_step_to_the_past(target, time_stack, grandmaster):
    """A correction can decrease vehicle time: a step back of the grandmaster is applied, whatever the leap flags say."""
    step_ns = -2 * 1_000_000_000

    with probing(target) as stream:
        wait_for_stream_reliable(stream)
        baseline = stream.latest()
        mark = len(stream.samples())

        grandmaster.step(step_ns)

        def stepped(sample):
            drift = drifts_from_grandmaster_pace_ns([baseline, sample])[1]
            return abs(drift - step_ns) < SYNC_TOLERANCE_NS

        wait_for_stream(
            stream,
            stepped,
            SYNC_TIMEOUT_S,
            f"vehicle time to follow the step (baseline {baseline})",
        )
        vehicle = [s["vehicle_time_ns"] for s in stream.samples()[mark - 1 :]]

    assert any(b < a for a, b in zip(vehicle, vehicle[1:])), (
        "vehicle time did not decrease"
    )


# --- precision of vehicle time -----------------------------------------------------------------------------------

# Reference value of non-AVB nodes (software timestamps) from the note of the requirement: 200 - 500 us.
PRECISION_BOUND_NS = 500_000


@add_test_properties(
    partially_verifies=[
        "feat_req__time__vehicle_time_sync_prec",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_342_RATE_DEVIATION_NOT_APPLIED_PRECISION
def test_deviation_from_grandmaster_stays_within_the_precision_bound(
    time_stack, probe_stream, grandmaster
):
    """While neither a timeout nor a time leap is indicated, no update corrects vehicle time by more than 500 us.

    Between two updates vehicle time advances with the local clock, so what an update corrects is the deviation that
    has built up since the last one. The absolute deviation from the grandmaster cannot be measured here (see
    test_vehicle_time_matches_grandmaster), and estimating it from the pace of the local clock is far less accurate
    than the bound.
    """
    wait_for_stream_in_sync(probe_stream)
    mark = len(probe_stream.samples())
    time.sleep(20)

    samples = probe_stream.samples()[mark:]
    assert len(samples) > 500
    # A sample pair that holds an update has a jump; the status of the later sample is the one of that update.
    corrections = [
        jump
        for later, jump in zip(samples[1:], vehicle_time_jumps_ns(samples))
        if not (later["timeout"] or later["leap_future"] or later["leap_past"])
    ]
    assert len(corrections) > 500
    worst = max(corrections, key=abs)
    assert abs(worst) < PRECISION_BOUND_NS, (
        f"an update corrected vehicle time by {worst / 1000:.0f} us"
    )


# --- snapshots are consistent ------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_snapshot_cons",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_snapshots_stay_consistent_while_the_state_changes(
    target, time_stack, probe_stream, grandmaster
):
    """Readers that are fast against the updates never see a status that contradicts itself, also across a timeout."""
    wait_for_stream_in_sync(probe_stream)
    with (
        probing(target, interval_ms=2) as fast_a,
        probing(target, interval_ms=2) as fast_b,
    ):
        grandmaster.pause_sync()
        time.sleep(TIMEOUT_S + 1.5)  # timeout
        grandmaster.resume_sync()
        time.sleep(2)  # recovery
        samples = fast_a.samples() + fast_b.samples() + probe_stream.samples()

    assert len(samples) > 300
    synchronized = [s for s in samples if s["synchronized"]]
    assert synchronized and any(s["timeout"] for s in synchronized)
    # The status is never empty once synchronized, and both leaps are never reported together; a status that
    # `consistent` rejects was combined from two different synchronization states.
    assert [s for s in synchronized if not s["consistent"]] == []
