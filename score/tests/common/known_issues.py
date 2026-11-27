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

"""The `pytest.mark.xfail` markers of the end-to-end tests, for the defects that they are known to expose.

Every expected failure of the tests is defined here, and nowhere else, so that the known defects are listed in
one place. A marker is named `XFAIL_<issue number>_<what is wrong>`.

A test is marked by applying the marker as a decorator, and a marker that is shared by several tests is a single
definition. Each reference names the GitHub issue and, where there is one, the pull request that fixes it.

`strict=True` is for a failure that is deterministic: the test then fails once the defect is fixed, to remind that
the marker has to be removed. `strict=False` is for a failure that depends on the platform (such as the clock of
the QEMU guest), so the test may legitimately pass.
"""

import pytest

_REPOSITORY = "https://github.com/eclipse-score/time"


def _issue(number):
    return f"{_REPOSITORY}/issues/{number}"


def _pull_request(number):
    return f"{_REPOSITORY}/pull/{number}"


def _xfail(reason, *, strict, raises=None):
    return pytest.mark.xfail(reason=reason, strict=strict, raises=raises)


# --- #226: domain number of the Pdelay messages (fix: PR #288) ----------------------------------------------------

XFAIL_226_PDELAY_REQ_OF_OTHER_DOMAIN_NOT_ANSWERED = _xfail(
    "time_slave only answers a Pdelay_Req of its configured domain, but has to ignore the domain "
    f"({_issue(226)}, fix: {_pull_request(288)})",
    strict=True,
    raises=TimeoutError,
)

XFAIL_226_PDELAY_MESSAGES_CARRY_CONFIGURED_DOMAIN = _xfail(
    "time_slave puts its configured domain number, not 0, in the Pdelay messages it sends "
    f"({_issue(226)}, fix: {_pull_request(288)})",
    strict=True,
)


# --- #257: gPTP timestamps on a NIC without hardware timestamps (fix: PR #285) -----------------------------
#
# The virtual NIC of the QEMU target has no hardware timestamps, neither for received nor for transmitted frames.

XFAIL_257_NO_SOFTWARE_TIMESTAMPS = _xfail(
    "needs the software timestamp fallback of NICs without hardware timestamping: the ingress timestamp of a Sync "
    "is 0, so neither the sync ingress time (needed for the time jump detection) nor the rate deviation are usable "
    f"({_issue(257)}, fix: {_pull_request(285)})",
    strict=False,
)

XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP = _xfail(
    "no Pdelay_Resp_Follow_Up is sent on a NIC without TX timestamps (virtio): the answer to a Pdelay_Req stays at "
    f"the Pdelay_Resp, and t3 is 0 or missing ({_issue(257)})",
    strict=False,
)

# On a NIC without TX timestamps a peer delay is never measured: PeerDelayMeasurer::SendRequest() keeps t1 as
# "pending" while RawSocketImpl::Send() waits up to 50 ms for a TX timestamp that does not come, the Pdelay_Resp
# arrives within that time, and the measurement that is dropped because of the pending t1 is not computed again.
XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP = _xfail(
    "time_slave never completes a peer delay measurement on a NIC without TX timestamps: the response arrives "
    f"while t1 is still pending, and the measurement is not recomputed ({_issue(257)})",
    strict=True,
    raises=TimeoutError,
)

XFAIL_257_PEER_DELAY_DISCARD_THRESHOLD = _xfail(
    "no peer delay is measured on a NIC without TX timestamps, and time_slave has no discard threshold: any "
    f"positive peer delay would be applied to the vehicle time ({_issue(257)}; the threshold has no issue yet)",
    strict=True,
)

XFAIL_257_PEER_DELAY_NOT_KEPT_OVER_RESTART = _xfail(
    "no peer delay is measured on a NIC without TX timestamps, and nothing in time_slave stores a peer delay, so a "
    f"restarted time slave would not have one either ({_issue(257)}; the persistence has no issue yet)",
    strict=True,
    raises=TimeoutError,
)


# --- #264: time jump detection of the time daemon (TimeJumpsValidator) ----------------------------------------------

# The time daemon's jump limit is a fixed 500 us (#267), and its jump detection (#264, finding 1) compares the local
# time between two Syncs with the grandmaster time between them, without compensating the rate of the local clock.
#
# On the QEMU target the jump detection does not run at all yet: the ingress timestamp of a Sync is 0 without hardware
# timestamps (#257), so the validator never sees an increasing time and logs "Current Sync T2 is less than previous"
# for every Sync. Tests that expect a jump or a leap fail for that reason (see XFAIL_257_NO_SOFTWARE_TIMESTAMPS),
# and tests that expect none pass trivially. They are kept marked for when the software timestamp fallback (#285)
# makes the detection work.
#
# Once it does, the jitter of the software timestamps is expected to matter. The rate of the QEMU guest does not: it
# differs from an unadjusted host by a few ppm, well below 1 us per Sync interval. The jitter has not been measured at
# the validator yet, but vehicle time is corrected by 0.3 - 0.45 ms at the 95th percentile, with outliers of 2 - 11 ms,
# which is close to, and sometimes above, the fixed limit. If so, the limit has to be configurable (#267) and chosen
# for the timestamp source. Not strict, as the tests may pass between the outliers. The exact limit is covered by the
# unit tests of the time jump validator.
XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE = _xfail(
    "the jump detection does not work without ingress timestamps (#257), and the fixed 500 us jump limit of the "
    f"time daemon may be below the jitter of software timestamps ({_issue(264)}, {_issue(267)})",
    strict=False,
)

XFAIL_264_FIRST_UPDATE_AFTER_TIMEOUT_EVALUATED = _xfail(
    "TimeJumpsValidator compares the first update after a timeout with the last one before it, so a step during "
    "the timeout is indicated as a time leap. Passes while the jump detection does not run at all, see the note "
    f"above ({_issue(264)})",
    strict=False,
)

XFAIL_264_SINGLE_TIME_LEAP_INDICATION = _xfail(
    "TimeJumpsValidator has a single time jump state: a leap to the past replaces a leap to the future, instead of "
    "both being healed on their own count (VehicleTimeStatus::IsConsistent even calls both contradictory) "
    f"({_issue(264)})",
    strict=True,
)


# --- #267: timeouts and parameters of the time daemon -----------------------------------------------------------------

XFAIL_267_NO_STARTUP_TIMEOUT = _xfail(
    "no timeout is indicated in the 15 s after the start of the feature when no Sync ever arrives, although the "
    f"default timeout is 3.3 s ({_issue(267)})",
    strict=False,
    raises=TimeoutError,
)

XFAIL_267_LEAP_THRESHOLD_NOT_CONFIGURABLE = _xfail(
    "the time slave reads jump_future_threshold_ns, but the time daemon compares with a fixed 500 us "
    f"({_issue(267)})",
    strict=False,
)


# --- #310: vehicle time is offset by the epoch of the local clock (fix: PR #329) --------------------------------------

XFAIL_310_VEHICLE_TIME_MIXES_CLOCKS = _xfail(
    "time_slave stamps local_time with CLOCK_MONOTONIC, but HighResSteadyClock is based on CLOCK_REALTIME on Linux "
    "and on ClockCycles() (about 0.4 s ahead of CLOCK_MONOTONIC) on QNX, so VehicleClock::Now() is off by that "
    f"difference ({_issue(310)}, fix: {_pull_request(329)})",
    strict=True,
)


# --- #314: configuration values are not validated (fix: PR #332) ---------------------------------------------------

XFAIL_314_CONFIG_VALUES_NOT_VALIDATED = _xfail(
    "time_slave applies configuration values without checking them against the schema (domain_number 0..127) "
    f"({_issue(314)}, fix: {_pull_request(332)})",
    strict=True,
)


# --- #315: the time daemon does not follow a restarted time slave (fix: PR #322) ------------------------------------

XFAIL_315_DAEMON_DOES_NOT_FOLLOW_RESTARTED_SLAVE = _xfail(
    "time_daemon connects to the shared memory of the time slave once and does not follow a restarted one "
    f"({_issue(315)}, fix: {_pull_request(322)})",
    strict=True,
)


# --- #342: rate deviation is not applied to vehicle time ------------------------------------------------------------

_RATE_DEVIATION_NOT_APPLIED = (
    "vehicle time is extrapolated between two updates with the local clock only: the rate deviation is published, "
    "but not applied (VehicleClockBackendImpl::GetSnapshot)"
)

XFAIL_342_RATE_DEVIATION_NOT_APPLIED = _xfail(
    f"{_RATE_DEVIATION_NOT_APPLIED} ({_issue(342)})",
    strict=True,
)

XFAIL_342_RATE_DEVIATION_NOT_APPLIED_PRECISION = _xfail(
    f"{_RATE_DEVIATION_NOT_APPLIED}, so an update corrects the rate error of the clock over a Sync interval. On the "
    "QEMU guest that error is below 1 us (a few ppm), so the corrections of up to about 2 ms that exceed the 500 us "
    "of the precision bound are not caused by the rate; most likely they come from the jitter of the local receive "
    f"time of the Syncs (#339, #257) ({_issue(342)})",
    strict=False,
)


# --- #344: range of the origin timestamp is not checked -------------------------------------------------------------

XFAIL_344_ORIGIN_TIMESTAMP_RANGE_NOT_CHECKED = _xfail(
    "time_slave only rejects timestamps that overflow int64 ns (TimestampToTmv): it accepts seconds beyond "
    f"2^32 - 1 and nanoseconds beyond 999 999 999, and treats an overflow as time 0 ({_issue(344)})",
    strict=True,
)


# --- #345: no Follow_Up receipt timeout -----------------------------------------------------------------------------

XFAIL_345_NO_FOLLOW_UP_RECEIPT_TIMEOUT = _xfail(
    "time_slave has no Follow_Up receipt timeout: it keeps a Sync until a Follow_Up arrives and uses it "
    f"({_issue(345)})",
    strict=True,
)


# --- #346: notification before the first Sync -----------------------------------------------------------------------

XFAIL_346_NOTIFIED_BEFORE_FIRST_SYNC = _xfail(
    "a subscription is served with the current snapshot at once, which is all zeros before the first Sync: an "
    f"update that never happened is notified ({_issue(346)})",
    strict=True,
)


# --- #347: notifications are lost when faster than the polling ------------------------------------------------------

XFAIL_347_NOTIFICATIONS_LOST_WHEN_FASTER_THAN_POLLING = _xfail(
    "the notifications are delivered by polling the latest snapshot every 50 ms (SvtCallbackDispatcher), so "
    f"updates that follow each other faster are lost ({_issue(347)})",
    strict=True,
)


# --- #348: Pdelay_Req warm-up exceeds the Sync interval -------------------------------------------------------------

XFAIL_348_PDELAY_WARMUP_EXCEEDS_SYNC_INTERVAL = _xfail(
    "the Pdelay_Req warm-up of the project configuration is 2 s, which is far above the synchronization interval "
    "of 125 ms that the first Pdelay_Req is required to follow the start by "
    f"({_issue(348)})",
    strict=True,
)
