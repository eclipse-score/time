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

"""Time leap indications."""

import time

from attribute_plugin import add_test_properties
from helpers import (
    LEAP_STEP_NS,
    SYNC_TIMEOUT_S,
    assert_no_jump_reported,
    status_key,
    wait_for_stream,
)
from known_issues import (
    XFAIL_257_NO_SOFTWARE_TIMESTAMPS,
    XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE,
    XFAIL_264_FIRST_UPDATE_AFTER_TIMEOUT_EVALUATED,
    XFAIL_264_SINGLE_TIME_LEAP_INDICATION,
)

# --- time leap indications ---------------------------------------------------------------------------------------

# Sync/Follow_Up pairs that follow each other this fast keep the error that the clock rate error of the host and the
# guest causes in the time jump detection (see XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE) below its limit of 500 us.
FAST_FRAME_INTERVAL_S = 0.01


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_leap_after_timeout",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_264_FIRST_UPDATE_AFTER_TIMEOUT_EVALUATED
def test_first_update_after_a_timeout_is_not_evaluated_for_a_time_leap(
    jump_detection_armed, grandmaster
):
    """The timeout already marks the discontinuity, so a step during it is not indicated as a time leap as well."""
    stream = jump_detection_armed
    grandmaster.pause_sync()
    wait_for_stream(
        stream, lambda s: s["timeout"], SYNC_TIMEOUT_S, "the timeout to be reported"
    )

    grandmaster.step(LEAP_STEP_NS)
    mark = len(stream.samples())
    grandmaster.send_sync_frames(2, interval_s=FAST_FRAME_INTERVAL_S)
    wait_for_stream(stream, lambda s: not s["timeout"], 5, "the timeout to end")
    time.sleep(1)
    samples = stream.samples()[mark:]

    assert samples
    assert_no_jump_reported(samples)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_leap_past_heal",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_264_SINGLE_TIME_LEAP_INDICATION
# Pytest applies only the xfail marker closest to the test (so, its strictness): without ingress timestamps no leap is
# detected at all, so this one comes first. Remove it when #257 is fixed, so that the strict marker above takes effect.
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
def test_both_time_leap_indications_can_be_active_at_the_same_time(
    jump_detection_armed, grandmaster
):
    """A leap to the future is not healed by one update, so a leap to the past that follows it is active as well."""
    stream = jump_detection_armed
    grandmaster.pause_sync()

    grandmaster.step(LEAP_STEP_NS)
    grandmaster.send_sync_frames(1)
    wait_for_stream(
        stream, lambda s: s["leap_future"], 5, "the leap to the future to be reported"
    )

    mark = len(stream.samples())
    grandmaster.step(-2 * LEAP_STEP_NS)
    grandmaster.send_sync_frames(1)
    time.sleep(1)

    samples = stream.samples()[mark:]
    assert any(s["leap_future"] and s["leap_past"] for s in samples), [
        status_key(s) for s in samples
    ][-5:]


# --- time leap to the past is healed -----------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_leap_past_heal",
        "feat_req__time__veh_leap_heal",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
@XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE
def test_time_leap_to_the_past_is_cleared_by_two_valid_frames(
    jump_detection_armed, grandmaster
):
    """One valid sync frame after a leap to the past is not enough to clear it, two are; no leap to the future shows."""
    stream = jump_detection_armed
    mark = len(stream.samples())

    grandmaster.pause_sync()
    grandmaster.step(-LEAP_STEP_NS)
    grandmaster.send_sync_frames(1)  # carries the jump
    wait_for_stream(
        stream, lambda s: s["leap_past"], 5, "the leap to the past to be reported"
    )

    grandmaster.send_sync_frames(1)  # first valid frame
    time.sleep(0.8)
    assert stream.latest()["leap_past"], (
        "the leap to the past was cleared by a single valid frame"
    )

    grandmaster.send_sync_frames(1)  # second consecutive valid frame
    wait_for_stream(
        stream,
        lambda s: not s["leap_past"],
        5,
        "the leap to the past to be cleared by the second frame",
    )
    assert not any(s["leap_future"] for s in stream.samples()[mark:])

    grandmaster.resume_sync()
    wait_for_stream(
        stream, lambda s: s["reliable"], 10, "vehicle time to be reliable again"
    )
