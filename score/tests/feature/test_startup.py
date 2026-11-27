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

"""Start-up behavior and latency of the time stack."""

import time

import pytest
from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    GrandMaster,
)
from helpers import (
    FIRST_SEQUENCE_ID,
    GM_IFACE,
    GM_OFFSET_NS,
    SYNC_INTERVAL_S,
    SYNC_TIMEOUT_S,
    TIMEOUT_S,
    measure_latency,
    probe_stalled_between,
    probing,
    running_time_stack,
    status_key,
    sync_events_after_first_sync,
    wait_for,
    wait_for_stream,
    wait_for_stream_in_sync,
)
from known_issues import (
    XFAIL_257_NO_SOFTWARE_TIMESTAMPS,
    XFAIL_267_NO_STARTUP_TIMEOUT,
)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_status_init",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_status_after_start_indicates_nothing(target, platform, gptp_iface):
    """Right after the start the status is neither synchronized, nor timeout, nor a time leap."""
    with running_time_stack(target, platform, gptp_iface):
        started = time.monotonic()
        with probing(target) as stream:
            wait_for(
                lambda: stream.latest() is not None,
                10,
                "the vehicle time probe to report",
            )
            if time.monotonic() - started > TIMEOUT_S - 1:
                pytest.skip(
                    "the probe took too long to start for the status to be checked before the timeout"
                )
            first = stream.samples()[0]

    assert status_key(first) == (False, False, False, False), first


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_startup_timeout",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_267_NO_STARTUP_TIMEOUT
def test_timeout_is_indicated_when_no_sync_arrives_after_start(
    time_stack, probe_stream
):
    """Without any Sync after the start of the feature the timeout is indicated, without synchronized."""
    wait_for_stream(
        probe_stream, lambda s: s["timeout"], 15, "the start-up timeout to be reported"
    )
    latest = probe_stream.latest()
    assert latest["timeout"] and not latest["synchronized"], latest


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_status_init",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_first_sync_after_start_is_processed(time_stack, event_probe_stream):
    """The time stack is ready before the network communicates: the very first Sync/Follow_Up pair is used."""
    # The first notification is for the update that never happened (see test_no_sync_update_is_notified_before...).
    time.sleep(1)
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, first_sequence_id=FIRST_SEQUENCE_ID
    ) as master:
        wait_for(
            lambda: sync_events_after_first_sync(event_probe_stream),
            SYNC_TIMEOUT_S,
            "the first update to be notified",
            lambda: f"events: {event_probe_stream.events()[:3]}",
        )
        first_sent = master.sync_log[0]["sequence_id"]
        first_notified = sync_events_after_first_sync(event_probe_stream)[0][
            "sequence_id"
        ]

    assert first_notified == first_sent, (
        f"the update of Sync {first_sent} was not used, the first one is {first_notified}"
    )


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_sync_latency",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
def test_synchronized_within_one_sync_interval_of_the_first_update(
    target, platform, gptp_iface
):
    """The synchronized indication is delivered less than one Sync interval after the Sync of the first update.

    Both timestamps are CLOCK_MONOTONIC: the ingress timestamp of the Sync and the SteadyClock of the sample.
    A measurement during which the probe was not scheduled is repeated, see measure_latency().
    """

    def trial():
        with (
            running_time_stack(target, platform, gptp_iface),
            probing(target, events=True) as stream,
            GrandMaster(
                GM_IFACE, offset_ns=GM_OFFSET_NS, first_sequence_id=FIRST_SEQUENCE_ID
            ),
        ):
            wait_for_stream_in_sync(stream)
            time.sleep(1)
            first_update = sync_events_after_first_sync(stream)[0]
            samples = stream.samples()
        first_synchronized = next(s for s in samples if s["synchronized"])
        ingress_ns = first_update["sync_ingress_ns"]
        synchronized_ns = first_synchronized["monotonic_time_ns"]
        return (synchronized_ns - ingress_ns) / 1e9, probe_stalled_between(
            samples, ingress_ns, synchronized_ns
        )

    latency_s = measure_latency(
        trial, SYNC_INTERVAL_S, "the latency of the synchronized indication"
    )
    assert 0 <= latency_s < SYNC_INTERVAL_S, (
        f"synchronized {latency_s * 1000:.1f} ms after the Sync"
    )
