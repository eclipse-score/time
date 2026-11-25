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

"""Peer delay measurement and its use."""

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
    PDELAY_INTERVAL_S,
    SYNC_INTERVAL_S,
    HostStallMonitor,
    assert_following_grandmaster,
    measure_latency,
    output_of,
    probing,
    remove_shm_files,
    running_time_stack,
    start_time_slave,
    vehicle_time_jumps_ns,
    wait_for,
    wait_for_stream_in_sync,
    wait_for_stream_reliable,
    wait_for_vehicle_time_jump,
    write_json_file,
)
from known_issues import (
    XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP,
    XFAIL_257_PEER_DELAY_DISCARD_THRESHOLD,
    XFAIL_257_PEER_DELAY_NOT_KEPT_OVER_RESTART,
    XFAIL_348_PDELAY_WARMUP_EXCEEDS_SYNC_INTERVAL,
)

# --- peer delay --------------------------------------------------------------------------------------------------

# Far above the delay of the virtual link, and large against the noise of a jump of the vehicle time.
LINK_DELAY_NS = 2_000_000_000

JUMP_TOLERANCE_NS = 400_000_000


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_pdelay_comp",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP
def test_link_delay_is_compensated(time_stack, probe_stream, grandmaster):
    """The peer delay that the time slave measures is added to the grandmaster time, and follows changes of it."""
    wait_for_stream_reliable(probe_stream)

    mark = len(probe_stream.samples())
    grandmaster.pdelay_extra_delay_ns = LINK_DELAY_NS
    wait_for_vehicle_time_jump(
        probe_stream, mark, LINK_DELAY_NS, "the link delay", JUMP_TOLERANCE_NS
    )

    mark = len(probe_stream.samples())
    grandmaster.pdelay_extra_delay_ns = 0
    wait_for_vehicle_time_jump(
        probe_stream,
        mark,
        -LINK_DELAY_NS,
        "the link delay being gone",
        JUMP_TOLERANCE_NS,
    )


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_pdelay_comp",
        "feat_req__time__veh_pdelay_discard",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@XFAIL_257_PEER_DELAY_DISCARD_THRESHOLD
def test_peer_delay_above_discard_threshold_is_not_applied(
    time_stack, probe_stream, grandmaster
):
    """A peer delay of 5 s is far above any discard threshold of a project, so it must not shift the vehicle time."""
    wait_for_stream_reliable(probe_stream)
    # Without this, the test would pass whenever no peer delay is applied at all.
    mark = len(probe_stream.samples())
    grandmaster.pdelay_extra_delay_ns = LINK_DELAY_NS
    wait_for_vehicle_time_jump(
        probe_stream, mark, LINK_DELAY_NS, "the link delay", JUMP_TOLERANCE_NS
    )

    mark = len(probe_stream.samples())
    grandmaster.pdelay_extra_delay_ns = 5_000_000_000
    # The delay is measured with the next Pdelay_Req and used by the next Sync.
    time.sleep(3 * PDELAY_INTERVAL_S)

    jumps = vehicle_time_jumps_ns(probe_stream.samples()[mark:])
    assert jumps and all(abs(jump) < JUMP_TOLERANCE_NS for jump in jumps), (
        f"vehicle time jumped by {max(jumps, key=abs)} ns"
    )


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_sync_no_pdelay",
    ],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
def test_synchronizes_when_peer_delay_is_not_answered(time_stack, probe_stream):
    """Without a Pdelay_Resp the time slave still synchronizes and keeps requesting."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, answer_pdelay=False) as master:
        wait_for_stream_reliable(probe_stream)
        mark = len(probe_stream.samples())
        requests = len(master.pdelay_request_times)
        time.sleep(4 * PDELAY_INTERVAL_S)
        samples = probe_stream.samples()[mark:]

        assert master.pdelay_count == 0
        assert len(master.pdelay_request_times) - requests >= 3, (
            "Pdelay_Req messages stopped"
        )
    assert_following_grandmaster(samples)


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_sync_no_pdelay",
        "feat_req__time__veh_ascapable_thresh",
    ],
    test_type="fault-injection",
    derivation_technique="boundary-values",
)
@pytest.mark.parametrize("delay_ns", [100_000_000, 2_000_000_000], ids=["100ms", "2s"])
def test_synchronizes_when_peer_delay_is_very_large(time_stack, probe_stream, delay_ns):
    """A peer delay far above neighborPropDelayThresh neither stops the synchronization nor the Pdelay_Req.

    The time slave does not measure the delay on every NIC (see XFAIL_257_NO_PEER_DELAY_WITHOUT_TX_TIMESTAMP): the test only shows
    that this does not matter, which is also what the requirement asks for when there is no valid measurement.
    """
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, pdelay_extra_delay_ns=delay_ns
    ) as master:
        wait_for_stream_reliable(probe_stream)
        requests = len(master.pdelay_request_times)
        time.sleep(4 * PDELAY_INTERVAL_S)
        latest = probe_stream.latest()

        assert master.pdelay_count > 0
        assert len(master.pdelay_request_times) - requests >= 3, (
            "Pdelay_Req messages stopped"
        )
    assert latest["synchronized"] and not latest["timeout"]


# --- first Pdelay_Req after start-up -----------------------------------------------------------------------------


def wait_for_log_line(handle, text, timeout_s):
    """Wait for `text` in the output of `handle` and return time.monotonic() of the poll that saw it."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        if text in output_of(handle):
            return time.monotonic()
        time.sleep(0.005)
    raise TimeoutError(f"{text!r} not logged within {timeout_s} s\n{output_of(handle)}")


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_startup_pdelay",
    ],
    test_type="requirements-based",
    derivation_technique="boundary-values",
)
@pytest.mark.parametrize(
    "warmup_ms",
    [
        pytest.param(0, id="without_warmup"),
        pytest.param(
            None,
            id="default_warmup",
            marks=XFAIL_348_PDELAY_WARMUP_EXCEEDS_SYNC_INTERVAL,
        ),
    ],
)
def test_first_pdelay_request_follows_the_start_within_one_sync_interval(
    target, platform, gptp_iface, warmup_ms
):
    """With the feature started and the link up, the first Pdelay_Req comes within one synchronization interval.

    The time of the start is the poll that saw the log line, which reaches the host over SSH, while the Pdelay_Req is
    stamped when it is received. The two differ by the SSH latency and the polling period, and the difference may be
    negative, so the check is only precise to a few milliseconds of the 125 ms bound and has no lower bound.
    """
    config = "/opt/etc/time_slave_config.json"
    if warmup_ms is not None:
        config = "/tmp/no_warmup_config.json"
        write_json_file(target, config, {"pdelay_warmup_ms": warmup_ms})

    def trial():
        remove_shm_files(target, platform)
        with (
            HostStallMonitor() as monitor,
            GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master,
        ):
            slave = start_time_slave(target, gptp_iface, config)
            try:
                started = wait_for_log_line(slave, "TimeSlave initialized", 10)
                wait_for(
                    lambda: master.pdelay_request_times, 10, "the first Pdelay_Req"
                )
                first = master.pdelay_request_times[0]
            finally:
                slave.stop()
                remove_shm_files(target, platform)
            # The start is only known to the poll that saw the log line, which may be late by the transport of
            # the line, so the window starts a little before it.
            return first - started, monitor.stalled_between(started - 0.1, first)

    latency_s = measure_latency(
        trial, SYNC_INTERVAL_S, "the delay of the first Pdelay_Req"
    )
    assert latency_s < SYNC_INTERVAL_S, (
        f"the first Pdelay_Req came {latency_s:.3f} s after the start"
    )


# --- peer delay is kept over a restart ---------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "feat_req__time__veh_pdelay_persist",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_PEER_DELAY_NOT_KEPT_OVER_RESTART
def test_peer_delay_is_used_after_a_restart_until_a_new_one_is_measured(
    target, platform, gptp_iface
):
    """A restarted time stack applies the last peer delay, while no Pdelay_Resp is received to measure a new one."""

    def offset_to_local_ns(stream):
        recent = stream.samples()[-10:]
        return statistics.mean(
            s["vehicle_time_ns"] - s["steady_time_ns"] for s in recent
        )

    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        with (
            running_time_stack(target, platform, gptp_iface),
            probing(target) as stream,
        ):
            wait_for_stream_in_sync(stream)
            mark = len(stream.samples())
            master.pdelay_extra_delay_ns = LINK_DELAY_NS
            wait_for_vehicle_time_jump(
                stream, mark, LINK_DELAY_NS, "the link delay", JUMP_TOLERANCE_NS
            )
            time.sleep(1)
            with_delay = offset_to_local_ns(stream)

        master.answer_pdelay = False
        with (
            running_time_stack(target, platform, gptp_iface),
            probing(target) as stream,
        ):
            wait_for_stream_in_sync(stream)
            time.sleep(1)
            restarted = offset_to_local_ns(stream)

    # The restarted stack must keep the delay: an offset that is off by about the whole delay is the offset
    # without a peer delay. The local clock may run fast, which accumulates in the offset over the restart, but far
    # less than the half of the delay.
    assert abs(restarted - with_delay) < LINK_DELAY_NS / 2, (
        f"{(restarted - with_delay) / 1e9:.2f} s off"
    )
