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


# --- #257: gPTP timestamps on a NIC without hardware timestamps (fix: PR #285) -----------------------------
#
# The virtual NIC of the QEMU target has no hardware timestamps, neither for received nor for transmitted frames.

XFAIL_257_NO_SOFTWARE_TIMESTAMPS = _xfail(
    "needs the software timestamp fallback of NICs without hardware timestamping: the ingress timestamp of a Sync "
    "is 0, so neither the sync ingress time (needed for the time jump detection) nor the rate deviation are usable "
    f"({_issue(257)}, fix: {_pull_request(285)})",
    strict=False,
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


# --- #267: timeouts and parameters of the time daemon -----------------------------------------------------------------

XFAIL_267_NO_STARTUP_TIMEOUT = _xfail(
    "no timeout is indicated in the 15 s after the start of the feature when no Sync ever arrives, although the "
    f"default timeout is 3.3 s ({_issue(267)})",
    strict=False,
    raises=TimeoutError,
)


# --- #310: vehicle time is offset by the epoch of the local clock (fix: PR #329) --------------------------------------

XFAIL_310_VEHICLE_TIME_MIXES_CLOCKS = _xfail(
    "time_slave stamps local_time with CLOCK_MONOTONIC, but HighResSteadyClock is based on CLOCK_REALTIME on Linux "
    "and on ClockCycles() (about 0.4 s ahead of CLOCK_MONOTONIC) on QNX, so VehicleClock::Now() is off by that "
    f"difference ({_issue(310)}, fix: {_pull_request(329)})",
    strict=True,
)


# --- #315: the time daemon does not follow a restarted time slave (fix: PR #322) ------------------------------------

XFAIL_315_DAEMON_DOES_NOT_FOLLOW_RESTARTED_SLAVE = _xfail(
    "time_daemon connects to the shared memory of the time slave once and does not follow a restarted one "
    f"({_issue(315)}, fix: {_pull_request(322)})",
    strict=True,
)
