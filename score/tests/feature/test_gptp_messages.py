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

"""Handling of the gPTP messages by the time slave, as seen on the vehicle time."""

import time

import pytest
from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    ETH_P_1588,
    MSG_PDELAY_REQ,
    MSG_PDELAY_RESP,
    MSG_PDELAY_RESP_FOLLOW_UP,
    PTP_MULTICAST_MAC,
    GrandMaster,
    set_origin_timestamp,
)
from helpers import (
    GM_IFACE,
    GM_OFFSET_NS,
    OTHER_DOMAIN,
    SYNC_TIMEOUT_S,
    UNUSABLE_SYNC_DWELL_S,
    assert_following_grandmaster,
    request_peer_delay,
    wait_for,
    wait_for_stream,
    wait_for_stream_reliable,
    wait_for_vehicle_time_jump,
)
from known_issues import (
    XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP,
    XFAIL_344_ORIGIN_TIMESTAMP_RANGE_NOT_CHECKED,
    XFAIL_345_NO_FOLLOW_UP_RECEIPT_TIMEOUT,
)

# --- gPTP message handling ----------------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__ethernet_address",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP
def test_time_slave_sends_untagged_frames_to_the_gptp_address(time_stack, grandmaster):
    """Every gPTP message of the time slave is an untagged frame to 01:80:C2:00:00:0E with Ethertype 0x88F7."""
    wait_for(
        lambda: len(grandmaster.pdelay_request_times) >= 3, 20, "3 Pdelay_Req messages"
    )
    request_peer_delay(grandmaster)

    frames = list(grandmaster.received_frames)
    assert {f["type"] for f in frames} >= {
        MSG_PDELAY_REQ,
        MSG_PDELAY_RESP,
        MSG_PDELAY_RESP_FOLLOW_UP,
    }
    assert all(
        f["destination"] == PTP_MULTICAST_MAC and f["ethertype"] == ETH_P_1588
        for f in frames
    )
    assert grandmaster.non_conforming_frames == []


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__sync_reception",
    ],
    test_type="requirements-based",
    derivation_technique="error-guessing",
)
def test_announce_and_signaling_messages_are_ignored(
    time_stack, probe_stream, grandmaster
):
    """Announce (of a better clock) and Signaling (asking to stop all messages) change nothing."""
    wait_for_stream_reliable(probe_stream)
    mark = len(probe_stream.samples())
    requests = len(grandmaster.pdelay_request_times)
    frames = len(grandmaster.received_frames)

    for _ in range(10):
        for domain in (None, OTHER_DOMAIN):
            grandmaster.send_announce(domain)
            grandmaster.send_signaling(domain)
        time.sleep(0.3)
    samples = probe_stream.samples()[mark:]

    assert_following_grandmaster(samples)
    assert len(grandmaster.pdelay_request_times) - requests >= 2, (
        "Pdelay_Req messages stopped"
    )
    # The time slave neither answers nor forwards them: it only sends the Pdelay messages.
    types = {f["type"] for f in grandmaster.received_frames[frames:]}
    assert types <= {MSG_PDELAY_REQ, MSG_PDELAY_RESP, MSG_PDELAY_RESP_FOLLOW_UP}, types


# --- content of the Sync/Follow_Up messages ------------------------------------------------------------------------

MAX_SECONDS = 2**32 - 1
MAX_NANOSECONDS = 999_999_999


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_tp_format",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
def test_origin_timestamp_at_the_range_limit_is_used(time_stack, probe_stream):
    """The largest preciseOriginTimestamp that is not out of range (2^32 - 1 s, 999 999 999 ns) is accepted."""
    wait_for(
        lambda: probe_stream.latest() is not None,
        10,
        "the vehicle time probe to report",
    )

    fault = {"mangle_follow_up": set_origin_timestamp(MAX_SECONDS, MAX_NANOSECONDS)}
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, fault=fault):
        wait_for_stream(
            probe_stream, lambda s: s["synchronized"], SYNC_TIMEOUT_S, "to synchronize"
        )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__followup_processing",
        "feat_req__time__veh_tp_format",
    ],
    test_type="fault-injection",
    derivation_technique="boundary-values",
)
@XFAIL_344_ORIGIN_TIMESTAMP_RANGE_NOT_CHECKED
@pytest.mark.parametrize(
    "seconds, nanoseconds",
    [
        pytest.param(MAX_SECONDS + 1, 0, id="seconds_2^32"),
        pytest.param(2**48 - 1, 0, id="seconds_2^48-1"),
        pytest.param(1000, MAX_NANOSECONDS + 1, id="nanoseconds_1e9"),
        pytest.param(1000, 0xFFFFFFFF, id="nanoseconds_2^32-1"),
    ],
)
def test_out_of_range_origin_timestamp_is_discarded(
    time_stack, probe_stream, seconds, nanoseconds
):
    """A Follow_Up whose preciseOriginTimestamp is out of range does not synchronize the vehicle time."""
    wait_for(
        lambda: probe_stream.latest() is not None,
        10,
        "the vehicle time probe to report",
    )

    # The synchronized flag stays set once reached, so the fault has to apply from the very first frame.
    with GrandMaster(
        GM_IFACE,
        offset_ns=GM_OFFSET_NS,
        fault={"mangle_follow_up": set_origin_timestamp(seconds, nanoseconds)},
    ):
        time.sleep(UNUSABLE_SYNC_DWELL_S)
        samples = probe_stream.samples()

    assert samples
    assert not [s for s in samples if s["synchronized"]], (
        "synchronized to an out of range timestamp"
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__sync_reception",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@pytest.mark.parametrize(
    "increment",
    [7919, 0xFFFF, 0x8000],
    ids=["jumps_forward", "steps_back_by_1", "jumps_by_half"],
)
def test_sequence_id_jumps_are_not_checked(
    time_stack, probe_stream, grandmaster, increment
):
    """The sequenceId of a Sync may differ from the previous one by any amount, also backwards."""
    wait_for_stream_reliable(probe_stream)
    mark = len(probe_stream.samples())

    grandmaster.sequence_id_increment = increment
    time.sleep(3)
    samples = probe_stream.samples()[mark:]

    assert_following_grandmaster(samples)
    assert grandmaster.sync_count > 0


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__followup_processing",
    ],
    test_type="fault-injection",
    derivation_technique="boundary-values",
)
@XFAIL_345_NO_FOLLOW_UP_RECEIPT_TIMEOUT
def test_follow_up_after_receipt_timeout_is_discarded(time_stack, probe_stream):
    """A Follow_Up that arrives 2 s after its Sync, far longer than any Follow_Up receipt timeout, is not used."""
    wait_for(
        lambda: probe_stream.latest() is not None,
        10,
        "the vehicle time probe to report",
    )

    # The synchronized flag stays set once reached, so the fault has to apply from the very first frame.
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, fault={"follow_up_delay_s": 2.0}
    ) as master:
        time.sleep(8)
        samples = probe_stream.samples()
        assert master.sync_count > 0

    assert samples
    assert not [s for s in samples if s["synchronized"]], (
        "synchronized to a Follow_Up that came too late"
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__followup_processing",
        "comp_req__time_slave__offset_calculation",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@pytest.mark.parametrize(
    "sync_correction_ns, follow_up_correction_ns",
    [
        pytest.param(100_000_000, 0, id="sync"),
        pytest.param(0, 100_000_000, id="follow_up"),
        pytest.param(-100_000_000, 0, id="negative"),
        pytest.param(60_000_000, 40_000_000, id="both_add_up"),
    ],
)
def test_correction_fields_are_added_to_the_origin_timestamp(
    time_stack, probe_stream, grandmaster, sync_correction_ns, follow_up_correction_ns
):
    """The correctionField of the Sync and of the Follow_Up are added to the preciseOriginTimestamp."""
    wait_for_stream_reliable(probe_stream)

    mark = len(probe_stream.samples())
    grandmaster.sync_correction_ns = sync_correction_ns
    grandmaster.follow_up_correction_ns = follow_up_correction_ns
    wait_for_vehicle_time_jump(
        probe_stream,
        mark,
        sync_correction_ns + follow_up_correction_ns,
        "the correction fields",
    )
