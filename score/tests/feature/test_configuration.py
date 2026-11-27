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

"""Effect of configured values."""

import time

from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    GrandMaster,
)
from helpers import (
    GM_IFACE,
    GM_OFFSET_NS,
    JUMP_DEBOUNCE_S,
    LEAP_STEP_NS,
    assert_no_jump_reported,
    assert_regular_gaps,
    probing,
    running_time_stack,
    wait_for,
    wait_for_stream,
    wait_for_stream_in_sync,
    write_json_file,
)
from known_issues import (
    XFAIL_267_LEAP_THRESHOLD_NOT_CONFIGURABLE,
)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__config_params",
        "feat_req__time__veh_pdelay_interval",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_pdelay_warmup_and_interval_follow_the_configuration(
    target, platform, gptp_iface
):
    """A configuration file sets the warm-up and the interval of the Pdelay_Req messages."""
    warmup_s, interval_s = 3.0, 0.4
    config = "/tmp/pdelay_timing_config.json"
    write_json_file(
        target,
        config,
        {
            "pdelay_warmup_ms": int(warmup_s * 1000),
            "pdelay_req_interval_ms": int(interval_s * 1000),
        },
    )
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        launched_at = time.monotonic()
        with running_time_stack(target, platform, gptp_iface, config=config):
            wait_for(
                lambda: len(master.pdelay_request_times) >= 6,
                20,
                "6 Pdelay_Req messages",
            )
            times = list(master.pdelay_request_times[:6])

    assert warmup_s <= times[0] - launched_at < warmup_s + 4
    gaps = [b - a for a, b in zip(times, times[1:])]
    assert_regular_gaps(gaps, interval_s)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__config_params",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_timeout_period_follows_the_configuration(target, platform, gptp_iface):
    """A shorter timeout period in the configuration file makes the timeout be indicated earlier than after 3.3 s."""
    config = "/tmp/short_timeout_config.json"
    write_json_file(target, config, {"sync_timeout_ms": 1000})
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        with (
            running_time_stack(target, platform, gptp_iface, config=config),
            probing(target) as stream,
        ):
            wait_for_stream_in_sync(stream)
            master.pause_sync()
            paused_at = time.monotonic()
            wait_for_stream(
                stream, lambda s: s["timeout"], 10, "the timeout to be indicated"
            )
            indicated_after = time.monotonic() - paused_at

    assert indicated_after < 2.5, (
        f"the timeout was indicated {indicated_after:.1f} s after the last Sync"
    )


@add_test_properties(
    partially_verifies=[
        "feat_req__time__config_params",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_267_LEAP_THRESHOLD_NOT_CONFIGURABLE
def test_time_leap_threshold_follows_the_configuration(target, platform, gptp_iface):
    """With a threshold of 1 s in the configuration file, a step of 100 ms is not a time leap."""
    config = "/tmp/high_leap_threshold_config.json"
    write_json_file(target, config, {"jump_future_threshold_ns": 1_000_000_000})
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        with (
            running_time_stack(target, platform, gptp_iface, config=config),
            probing(target) as stream,
        ):
            wait_for_stream_in_sync(stream)
            time.sleep(JUMP_DEBOUNCE_S + 2)
            mark = len(stream.samples())
            master.step(LEAP_STEP_NS)
            time.sleep(3)
            samples = stream.samples()[mark:]

    assert samples
    assert_no_jump_reported(samples)
