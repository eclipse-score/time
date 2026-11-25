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

"""Component requirements of the time daemon."""

import time

import pytest
from attribute_plugin import add_test_properties
from helpers import (
    JUMP_DEBOUNCE_S,
    JUMP_THRESHOLD_NS,
    LEAP_FLAGS,
    SYNC_TOLERANCE_NS,
    TIMEOUT_S,
    assert_no_jump_reported,
    probing,
    sample_vehicle_time,
    wait_for,
    wait_for_stream,
    wait_for_stream_in_sync,
    wait_for_stream_reliable,
)
from known_issues import (
    XFAIL_257_NO_SOFTWARE_TIMESTAMPS,
    XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE,
)

# --- timeout -----------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__timeout_detection",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
def test_no_timeout_before_timeout_period_elapsed(
    time_stack, probe_stream, grandmaster
):
    """A gap in the sync messages that is shorter than the timeout period (3.3 s) is not a timeout."""
    wait_for_stream_reliable(probe_stream)

    mark = len(probe_stream.samples())
    grandmaster.pause_sync()
    time.sleep(TIMEOUT_S - 1.3)
    grandmaster.resume_sync()
    time.sleep(1)

    samples = probe_stream.samples()[mark:]
    assert samples
    assert not [s for s in samples if s["timeout"]], (
        "timeout reported before the timeout period elapsed"
    )
    assert probe_stream.latest()["reliable"]


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__timeout_detection",
        "comp_req__time_daemon__timeout_reaction",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
def test_timeout_after_timeout_period_elapsed(time_stack, probe_stream, grandmaster):
    """The timeout is reported once no sync message arrived for the timeout period, and cleared on resume."""
    wait_for_stream_reliable(probe_stream)

    grandmaster.pause_sync()
    paused_at = time.monotonic()
    wait_for_stream(probe_stream, lambda s: s["timeout"], 10, "timeout to be reported")
    detected_after_s = time.monotonic() - paused_at

    # The last sync message may have been sent up to one sync interval before the pause.
    assert detected_after_s >= TIMEOUT_S - 0.5
    latest = probe_stream.latest()
    # Reaction: time data is still published, with the timeout flag set.
    assert latest["timeout"] and not latest["reliable"]

    grandmaster.resume_sync()
    wait_for_stream_in_sync(probe_stream)


# --- time jumps ---------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__time_jump_detection",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE
def test_step_below_jump_threshold_is_not_reported(jump_detection_armed, grandmaster):
    """A step well below the 500 us limit of the time jump detection does not set a time jump flag."""
    stream = jump_detection_armed
    mark = len(stream.samples())

    grandmaster.step(JUMP_THRESHOLD_NS * 2 // 5)
    time.sleep(3)

    samples = stream.samples()[mark:]
    assert len(samples) > 10
    assert_no_jump_reported(samples)
    assert all(s["reliable"] for s in samples)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__time_jump_detection",
        "comp_req__time_daemon__time_jump_reaction",
        "comp_req__time_daemon__time_jump_recovery",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
@XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE
@pytest.mark.parametrize(
    "step_ns, flag",
    [
        pytest.param(
            3 * JUMP_THRESHOLD_NS, "leap_future", id="just_above_limit_forward"
        ),
        pytest.param(
            -3 * JUMP_THRESHOLD_NS, "leap_past", id="just_above_limit_backward"
        ),
        pytest.param(100_000_000, "leap_future", id="large_forward"),
        pytest.param(-100_000_000, "leap_past", id="large_backward"),
    ],
)
def test_step_above_jump_threshold_is_reported_and_cleared(
    jump_detection_armed, grandmaster, step_ns, flag
):
    """Forward and backward jumps are reported while time data keeps being published, and are cleared again."""
    stream = jump_detection_armed
    other_flag = next(f for f in LEAP_FLAGS if f != flag)
    mark = len(stream.samples())

    grandmaster.step(step_ns)

    wait_for(
        lambda: any(s[flag] for s in stream.samples()[mark:]),
        10,
        f"{flag} to be reported",
    )
    reported = [s for s in stream.samples()[mark:] if s[flag]]
    assert not any(s[other_flag] for s in stream.samples()[mark:])
    # Reaction: the time data is still published, synchronized, but not reliable.
    assert all(
        s["synchronized"] and not s["timeout"] and not s["reliable"] for s in reported
    )
    count = len(stream.samples())
    time.sleep(0.5)
    assert len(stream.samples()) > count

    # Recovery: sync continues without further jump.
    wait_for_stream(
        stream, lambda s: s["reliable"] and not s[flag], 10, "time jump to be cleared"
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__time_jump_recovery",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
@XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE
def test_time_jump_is_cleared_by_two_valid_frames(jump_detection_armed, grandmaster):
    """One valid sync frame after a time jump is not enough to clear it, two consecutive ones are."""
    stream = jump_detection_armed

    grandmaster.pause_sync()
    grandmaster.step(5_000_000)
    grandmaster.send_sync_frames(1)  # carries the jump
    wait_for_stream(stream, lambda s: s["leap_future"], 5, "time jump to be reported")

    grandmaster.send_sync_frames(1)  # first valid frame
    time.sleep(0.8)
    assert stream.latest()["leap_future"], "time jump cleared by a single valid frame"

    grandmaster.send_sync_frames(1)  # second consecutive valid frame
    wait_for_stream(
        stream,
        lambda s: not s["leap_future"],
        5,
        "time jump to be cleared by the second valid frame",
    )

    grandmaster.resume_sync()
    wait_for_stream(
        stream, lambda s: s["reliable"], 10, "vehicle time to be reliable again"
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__time_jump_detection",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
def test_time_jump_is_not_reported_during_startup_period(
    time_stack, probe_stream, grandmaster
):
    """Within 5 s after the initial synchronization no time jump is reported; afterwards it is."""
    wait_for_stream_reliable(probe_stream)
    synchronized_at = time.monotonic()

    mark = len(probe_stream.samples())
    grandmaster.step(5_000_000)
    time.sleep(2)
    samples = probe_stream.samples()[mark:]
    assert time.monotonic() - synchronized_at < JUMP_DEBOUNCE_S, (
        "start-up period already over, test is invalid"
    )
    assert samples
    assert_no_jump_reported(samples)

    time.sleep(max(0.0, synchronized_at + JUMP_DEBOUNCE_S + 2 - time.monotonic()))
    mark = len(probe_stream.samples())
    grandmaster.step(5_000_000)
    wait_for(
        lambda: any(s["leap_future"] for s in probe_stream.samples()[mark:]),
        10,
        "time jump to be reported",
    )


# --- clients -----------------------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_daemon__multi_client",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_multiple_clients_read_vehicle_time_concurrently(
    target, time_stack, grandmaster
):
    """Concurrent clients see the same vehicle time, and a client that goes away does not disturb the others."""

    def offset_to_steady_ns(sample):
        return sample["vehicle_time_ns"] - sample["steady_time_ns"]

    with probing(target) as remaining:
        wait_for_stream_reliable(remaining)
        with probing(target) as leaving:
            wait_for_stream_reliable(leaving)
            (one_shot,) = sample_vehicle_time(target)
            assert one_shot["reliable"]
            assert offset_to_steady_ns(remaining.latest()) == pytest.approx(
                offset_to_steady_ns(leaving.latest()), abs=SYNC_TOLERANCE_NS
            )

        mark = len(remaining.samples())
        time.sleep(1)
        assert len(remaining.samples()) > mark
        assert remaining.latest()["reliable"]
