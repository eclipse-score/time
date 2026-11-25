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

"""Notifications of status changes, synchronization updates and peer delay measurements."""

import time

from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    GrandMaster,
)
from helpers import (
    FIRST_SEQUENCE_ID,
    GM_IFACE,
    GM_OFFSET_NS,
    PDELAY_INTERVAL_S,
    SYNC_TIMEOUT_S,
    status_key,
    sync_events_after_first_sync,
    wait_for,
    wait_for_stream_in_sync,
    wait_for_stream_reliable,
)
from known_issues import (
    XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP,
    XFAIL_257_NO_SOFTWARE_TIMESTAMPS,
    XFAIL_346_NOTIFIED_BEFORE_FIRST_SYNC,
    XFAIL_347_NOTIFICATIONS_LOST_WHEN_FASTER_THAN_POLLING,
)


def distinct_in_a_row(items):
    return [item for i, item in enumerate(items) if i == 0 or item != items[i - 1]]


def is_subsequence(wanted, items):
    remaining = iter(items)
    return all(any(item == w for item in remaining) for w in wanted)


# --- notification of status changes -----------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_status_notify",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_status_changes_are_notified(time_stack, event_probe_stream, grandmaster):
    """Every change of the status that the application reads is also notified, in the same order."""
    stream = event_probe_stream
    wait_for_stream_reliable(stream)
    mark = len(stream.samples())

    grandmaster.pause_sync()
    wait_for(
        lambda: any(e["timeout"] for e in stream.events("status")),
        SYNC_TIMEOUT_S,
        "the timeout notification",
    )
    grandmaster.resume_sync()
    wait_for(
        lambda: not stream.events("status")[-1]["timeout"],
        SYNC_TIMEOUT_S,
        "the end of the timeout to be notified",
    )

    read = distinct_in_a_row([status_key(s) for s in stream.samples()[mark:]])
    notified = distinct_in_a_row([status_key(e) for e in stream.events("status")])
    assert (True, True, False, False) in read, "the probe did not read the timeout"
    assert is_subsequence(read, notified), f"read {read}, notified {notified}"


def matching_sync_log(grandmaster, event):
    (entry,) = [
        e for e in grandmaster.sync_log if e["sequence_id"] == event["sequence_id"]
    ] or [None]
    return entry


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_data_notify",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_346_NOTIFIED_BEFORE_FIRST_SYNC
def test_no_sync_update_is_notified_before_the_first_sync(
    time_stack, event_probe_stream
):
    """Without a grandmaster there is no time synchronization update to notify."""
    time.sleep(5)
    assert event_probe_stream.latest() is not None
    assert event_probe_stream.events("sync") == []


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_data_notify",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_every_sync_update_is_notified_once(time_stack, event_probe_stream):
    """Each Sync/Follow_Up pair of the grandmaster is notified, once, in the order of the pairs."""
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, first_sequence_id=FIRST_SEQUENCE_ID
    ) as master:
        wait_for_stream_reliable(event_probe_stream)
        time.sleep(5)

        events = sync_events_after_first_sync(event_probe_stream)
        ids = [e["sequence_id"] for e in events]
        sent = [entry["sequence_id"] for entry in master.sync_log]
    assert len(ids) > 10
    assert len(ids) == len(set(ids)), "an update was notified more than once"
    assert set(ids) <= set(sent), (
        f"notified, but not sent: {sorted(set(ids) - set(sent))}"
    )
    assert ids == sorted(ids, key=sent.index), "updates were notified out of order"
    missed = [i for i in sent[sent.index(ids[0]) : sent.index(ids[-1])] if i not in ids]
    assert not missed, f"updates that were not notified: {missed}"


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_data_notify",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_347_NOTIFICATIONS_LOST_WHEN_FASTER_THAN_POLLING
def test_every_sync_update_is_notified_when_syncs_are_frequent(
    time_stack, event_probe_stream
):
    """With a Sync every 20 ms (802.1AS allows 7.8 ms), each update is still notified."""
    with GrandMaster(
        GM_IFACE,
        offset_ns=GM_OFFSET_NS,
        sync_interval_s=0.02,
        first_sequence_id=FIRST_SEQUENCE_ID,
        tx_timestamp_wait_s=0,
    ) as master:
        wait_for_stream_in_sync(event_probe_stream)
        time.sleep(5)

        ids = [
            e["sequence_id"] for e in sync_events_after_first_sync(event_probe_stream)
        ]
        sent = [entry["sequence_id"] for entry in master.sync_log]
    assert len(sent) > 150, f"the grandmaster only sent {len(sent)} Syncs in about 5 s"
    missed = [i for i in sent[sent.index(ids[0]) : sent.index(ids[-1])] if i not in ids]
    assert not missed, f"{len(missed)} of {len(sent)} updates were not notified"


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_data_notify",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_SOFTWARE_TIMESTAMPS
def test_sync_update_data(time_stack, event_probe_stream):
    """The data of an update tells where it came from and what the grandmaster sent."""
    correction_ns = 7_000_000
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, first_sequence_id=FIRST_SEQUENCE_ID
    ) as master:
        master.sync_correction_ns = correction_ns
        wait_for_stream_reliable(event_probe_stream)
        time.sleep(3)
        events = sync_events_after_first_sync(event_probe_stream)
        gm_identity = master.port_identity
        sent_by_id = {e["sequence_id"]: e for e in master.sync_log}
    assert len(events) > 10

    for event in events:
        sent = sent_by_id.get(event["sequence_id"])
        assert sent is not None, (
            f"no Sync with sequence id {event['sequence_id']} was sent"
        )
        assert event["precise_origin_ns"] == sent["origin_ns"]
        # correctionField is in units of 2^-16 ns; the first updates can be from before the correction was set.
        assert event["correction_field"] in (0, correction_ns << 16)
        # preciseOriginTimestamp + correctionField is the grandmaster time of the Sync (no link delay is measured).
        assert (
            abs(
                event["reference_global_ns"]
                - sent["origin_ns"]
                - (event["correction_field"] >> 16)
            )
            < 1_000_000
        )
        assert event["source_port"] == int.from_bytes(gm_identity[8:], "big")
        # The clock identity is a number: either byte order of the 8 octets is a representation of it.
        assert event["source_clock_identity"] in (
            int.from_bytes(gm_identity[:8], "big"),
            int.from_bytes(gm_identity[:8], "little"),
        )
    assert any(e["correction_field"] == correction_ns << 16 for e in events), (
        "the correction field is not delivered"
    )

    ingress = [e["sync_ingress_ns"] for e in events]
    assert all(0 < b - a < 500_000_000 for a, b in zip(ingress, ingress[1:])), (
        "ingress timestamps do not advance"
    )


# --- notification of peer delay measurements -----------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_pdelay_notify",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP
def test_every_peer_delay_measurement_is_notified_once(
    time_stack, event_probe_stream, grandmaster
):
    """Each completed peer delay measurement is notified, with the timestamps that were exchanged."""
    wait_for(
        lambda: len(event_probe_stream.events("pdelay")) >= 3,
        20,
        "3 peer delay notifications",
    )
    time.sleep(2 * PDELAY_INTERVAL_S)

    events = event_probe_stream.events("pdelay")
    ids = [e["sequence_id"] for e in events]
    assert len(ids) == len(set(ids)), "a measurement was notified more than once"
    for event in events:
        (answered,) = [
            a
            for a in grandmaster.pdelay_log
            if a["sequence_id"] == event["sequence_id"]
        ]
        assert event["request_receipt_ns"] == answered["request_receipt_ns"]
        assert event["response_origin_ns"] == answered["response_origin_ns"]
        assert event["request_origin_ns"] < event["response_receipt_ns"]
        assert event["pdelay_ns"] >= 0
        assert event["response_port"] == int.from_bytes(
            grandmaster.port_identity[8:], "big"
        )
