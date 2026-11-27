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

"""Component requirements of the time slave."""

import time

import pytest
from attribute_plugin import add_test_properties
from gptp_grandmaster import (
    PTP_MULTICAST_MAC,
    GrandMaster,
    header_only,
    set_byte,
    set_message_length,
    truncate_to,
)
from helpers import (
    GM_IFACE,
    GM_OFFSET_NS,
    OTHER_DOMAIN,
    PDELAY_INTERVAL_S,
    PDELAY_WARMUP_S,
    SYNC_TOLERANCE_NS,
    UNUSABLE_SYNC_DWELL_S,
    assert_regular_gaps,
    assert_no_jump_reported,
    drifts_from_grandmaster_pace_ns,
    log_of,
    output_of,
    remove_shm_files,
    request_peer_delay,
    running_time_stack,
    sample_vehicle_time,
    start_time_slave,
    wait_for,
    wait_for_stream_reliable,
    wait_until_synchronized,
    write_json_file,
    write_text_file,
)
from known_issues import (
    XFAIL_226_PDELAY_MESSAGES_CARRY_CONFIGURED_DOMAIN,
    XFAIL_226_PDELAY_REQ_OF_OTHER_DOMAIN_NOT_ANSWERED,
    XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP,
    XFAIL_314_CONFIG_VALUES_NOT_VALIDATED,
)

# --- gPTP message handling of the time slave ---------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__domain_filtering",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_ignores_sync_of_other_domain(time_stack, probe_stream):
    """Sync/Follow_Up of a domain other than the configured one are ignored; Pdelay_Req uses domain 0."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, domain=5) as other_domain:
        time.sleep(PDELAY_WARMUP_S + 6)
        assert other_domain.sync_count > 0
        sample = probe_stream.latest()
        assert sample is not None
        assert not sample["synchronized"] and not sample["reliable"]
        # The time slave sets domain number 0 in the Pdelay messages it sends, whatever the master uses.
        assert other_domain.pdelay_request_times == []


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__domain_filtering",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_follows_only_configured_domain(time_stack, probe_stream, grandmaster):
    """With a second master in another domain, vehicle time keeps following the configured domain."""
    wait_for_stream_reliable(probe_stream)
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS + 100 * 1_000_000_000, domain=5):
        mark = len(probe_stream.samples())
        # Short enough to stay within the start-up period of the jump detection (5 s).
        time.sleep(3)
        samples = probe_stream.samples()[mark:]

    assert len(samples) > 10
    assert_no_jump_reported(samples)
    assert all(s["reliable"] for s in samples)
    assert all(
        abs(drift) < SYNC_TOLERANCE_NS
        for drift in drifts_from_grandmaster_pace_ns(samples)
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__followup_processing",
    ],
    test_type="requirements-based",
    derivation_technique="error-guessing",
)
def test_follow_up_with_non_matching_sequence_id_is_ignored(target, time_stack):
    """Follow_Up messages are matched to their Sync by sequence id; without a match no time is taken over."""
    # Corrupted from the first message on: the synchronized flag stays set once it has been reached.
    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, follow_up_sequence_id_offset=1000
    ) as master:
        time.sleep(8)
        (sample,) = sample_vehicle_time(target)
        assert master.sync_count > 0
        assert not sample["synchronized"] and not sample["reliable"]

        master.corrupt_follow_up_sequence_id(0)
        wait_until_synchronized(target)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req",
        "feat_req__time__veh_pdelay_interval",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_pdelay_requests_start_after_warmup_and_repeat_at_interval(
    target, platform, gptp_iface
):
    """First Pdelay_Req after the configured warm-up (2 s), then one per configured interval (1 s)."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        launched_at = time.monotonic()
        with running_time_stack(target, platform, gptp_iface):
            wait_for(
                lambda: len(master.pdelay_request_times) >= 5,
                20,
                "5 Pdelay_Req messages",
            )
            times = list(master.pdelay_request_times[:5])

    assert times[0] - launched_at >= PDELAY_WARMUP_S
    assert times[0] - launched_at < PDELAY_WARMUP_S + 8
    gaps = [b - a for a, b in zip(times, times[1:])]
    # feat_req__time__veh_pdelay_interval: every gap within +-25 % of the configured interval.
    assert_regular_gaps(gaps, PDELAY_INTERVAL_S)


# --- configuration of the time slave ------------------------------------------------------------------------------


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_missing_config_file_uses_defaults(target, platform, gptp_iface, grandmaster):
    """Without a configuration file the time slave uses defaults and still synchronizes."""
    with running_time_stack(
        target, platform, gptp_iface, config="/tmp/does_not_exist.json"
    ) as (slave, _):
        wait_until_synchronized(target)
        assert "using built-in defaults" in output_of(slave)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
        "comp_req__time_slave__error_reporting",
    ],
    test_type="fault-injection",
    derivation_technique="equivalence-classes",
)
def test_malformed_config_file_uses_defaults(target, platform, gptp_iface, grandmaster):
    """A malformed configuration file is reported and defaults are used, so the time slave still synchronizes."""
    exit_code, output = target.execute(
        "echo '{ \"iface_name\": ' > /tmp/malformed.json"
    )
    assert exit_code == 0, output
    with running_time_stack(
        target, platform, gptp_iface, config="/tmp/malformed.json"
    ) as (slave, _):
        wait_until_synchronized(target)
        assert "Failed to parse config file" in output_of(slave)


@pytest.fixture
def other_domain_config(target):
    """Time slave configuration file that only sets the domain number, to OTHER_DOMAIN."""
    path = "/tmp/other_domain_config.json"
    write_json_file(target, path, {"domain_number": OTHER_DOMAIN})
    return path


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_config_file_gives_only_the_configured_items(
    target, platform, gptp_iface, other_domain_config
):
    """Items missing in the configuration file keep their defaults; the given domain number is used."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, domain=OTHER_DOMAIN):
        with running_time_stack(
            target, platform, gptp_iface, config=other_domain_config
        ) as (slave, _):
            wait_until_synchronized(target)
            assert f"Loaded config from {other_domain_config}" in log_of(slave)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_config_file_from_environment_variable(
    target, platform, gptp_iface, other_domain_config
):
    """Without --config the file named by TIMESLAVE_CONFIG is used."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, domain=OTHER_DOMAIN):
        with running_time_stack(
            target,
            platform,
            gptp_iface,
            config=None,
            slave_env=f"TIMESLAVE_CONFIG={other_domain_config}",
        ):
            wait_until_synchronized(target)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_command_line_config_takes_precedence_over_environment(
    target, platform, gptp_iface, other_domain_config, grandmaster
):
    """--config wins over TIMESLAVE_CONFIG: the domain of the environment's file would not match the grandmaster."""
    with running_time_stack(
        target,
        platform,
        gptp_iface,
        slave_env=f"TIMESLAVE_CONFIG={other_domain_config}",
    ) as (
        slave,
        _,
    ):
        wait_until_synchronized(target)
        assert "Loaded config from /opt/etc/time_slave_config.json" in log_of(slave)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_default_config_path_is_used_without_other_source(
    target, platform, gptp_iface, grandmaster
):
    """Without --config and TIMESLAVE_CONFIG the time slave reads ./etc/time_slave_config.json of its working dir."""
    with running_time_stack(target, platform, gptp_iface, config=None) as (slave, _):
        wait_until_synchronized(target)
        assert "Loaded config from ./etc/time_slave_config.json" in log_of(slave)


# What the time slave logs for a configuration it cannot use, and how often (the same problem in several items).
INVALID_CONFIGS = {
    "empty_file": ("", "Failed to parse config file", 1),
    "array_as_root": ("[1, 2]", "root is not a JSON object", 1),
    "wrong_value_types": (
        '{"domain_number": "zero", "iface_name": 5, "sync_timeout_ms": "fast"}',
        "has wrong type",
        3,
    ),
    "sections_not_objects": ('{"phc": 5, "qnx": "x"}', "is not an object", 2),
}


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
        "comp_req__time_slave__error_reporting",
    ],
    test_type="fault-injection",
    derivation_technique="equivalence-classes",
)
@pytest.mark.parametrize(
    "content, problem, count", INVALID_CONFIGS.values(), ids=INVALID_CONFIGS.keys()
)
def test_invalid_config_file_is_reported_and_defaults_are_used(
    target, platform, gptp_iface, grandmaster, content, problem, count
):
    """Each unusable part of a configuration file is reported, and the time slave still synchronizes on defaults."""
    write_text_file(target, "/tmp/invalid_config.json", content)
    with running_time_stack(
        target, platform, gptp_iface, config="/tmp/invalid_config.json"
    ) as (slave, _):
        wait_until_synchronized(target)
        assert log_of(slave).count(problem) == count


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
        "comp_req__time_slave__error_reporting",
    ],
    test_type="fault-injection",
    derivation_technique="equivalence-classes",
)
def test_config_path_that_is_a_directory_is_reported_and_defaults_are_used(
    target, platform, gptp_iface
):
    """A configuration path that cannot be read as a file is handled like a malformed file."""
    remove_shm_files(target, platform)
    slave = start_time_slave(target, gptp_iface, config="/tmp")
    try:
        wait_for(
            lambda: (
                "TimeSlave initialized" in output_of(slave) or not slave.is_running()
            ),
            10,
            "time_slave to initialize or exit",
            lambda: output_of(slave),
        )
        assert slave.is_running(), f"time_slave terminated: {log_of(slave)}"
        assert "Failed to parse config file" in log_of(slave)
    finally:
        slave.stop()
        remove_shm_files(target, platform)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__configuration",
        "comp_req__time_slave__error_reporting",
    ],
    test_type="fault-injection",
    derivation_technique="boundary-values",
)
@XFAIL_314_CONFIG_VALUES_NOT_VALIDATED
def test_out_of_range_config_value_is_reported_and_default_is_used(
    target, platform, gptp_iface, grandmaster
):
    """A value outside the range of the configuration schema is reported, and the default is used instead."""
    write_json_file(target, "/tmp/out_of_range_config.json", {"domain_number": 128})
    with running_time_stack(
        target, platform, gptp_iface, config="/tmp/out_of_range_config.json"
    ) as (slave, _):
        assert "domain_number" in log_of(slave), "out-of-range value is not reported"
        wait_until_synchronized(target)


# --- gPTP frames the time slave has to reject ------------------------------------------------------------------------

# Each fault makes every Sync/Follow_Up pair unusable in another way; none of them may result in synchronization.
UNUSABLE_SYNC_FAULTS = {
    "follow_up_never_sent": {"send_follow_up": False},
    "sync_truncated_below_header": {"mangle_sync": truncate_to(20)},
    "follow_up_truncated_below_header": {"mangle_follow_up": truncate_to(20)},
    "follow_up_declared_length_exceeds_frame": {
        "mangle_follow_up": set_message_length(200)
    },
    "sync_with_wrong_ptp_version": {"mangle_sync": set_byte(1, 0x01)},
    "follow_up_with_wrong_transport_specific": {"mangle_follow_up": set_byte(0, 0x08)},
}


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__sync_reception",
        "comp_req__time_slave__followup_processing",
    ],
    test_type="fault-injection",
    derivation_technique="error-guessing",
)
def test_unusable_sync_frames_do_not_synchronize(time_stack, probe_stream):
    """Sync/Follow_Up pairs that are incomplete or malformed are dropped; the time slave survives them and recovers."""
    slave, daemon = time_stack
    wait_for(
        lambda: probe_stream.latest() is not None,
        10,
        "the vehicle time probe to report",
    )

    # The synchronized flag stays set once reached, so the faults have to apply from the very first frame.
    first_fault = next(iter(UNUSABLE_SYNC_FAULTS.values()))
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, fault=first_fault) as master:
        for name, fault in UNUSABLE_SYNC_FAULTS.items():
            master.inject_fault(**fault)
            mark = len(probe_stream.samples())
            time.sleep(UNUSABLE_SYNC_DWELL_S)
            samples = probe_stream.samples()[mark:]
            assert samples, f"{name}: no vehicle time samples"
            assert not [s for s in samples if s["synchronized"]], (
                f"{name}: synchronized to an unusable frame"
            )
        assert slave.is_running() and daemon.is_running()

        master.inject_fault()
        wait_for_stream_reliable(probe_stream)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__followup_processing",
    ],
    test_type="fault-injection",
    derivation_technique="error-guessing",
)
def test_follow_up_without_timestamp_is_not_used(time_stack, probe_stream):
    """A Follow_Up that has a valid header but lacks the origin timestamp must not set the time to the epoch."""
    wait_for(
        lambda: probe_stream.latest() is not None,
        10,
        "the vehicle time probe to report",
    )

    with GrandMaster(
        GM_IFACE, offset_ns=GM_OFFSET_NS, fault={"mangle_follow_up": header_only}
    ):
        time.sleep(UNUSABLE_SYNC_DWELL_S)
        samples = probe_stream.samples()
        assert samples
        assert not [s for s in samples if s["synchronized"]], (
            "synchronized to a Follow_Up without timestamp"
        )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req_response",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP
def test_time_slave_answers_pdelay_request_of_peer(time_stack):
    """A Pdelay_Req is answered by Pdelay_Resp and Pdelay_Resp_Follow_Up to the gPTP address, for the requester."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        replies = request_peer_delay(master)

    for reply in replies:
        assert reply["destination"] == PTP_MULTICAST_MAC
        assert reply["requesting_port"] == master.port_identity


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req_response",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP
def test_pdelay_response_receipt_timestamps_advance_with_time(time_stack):
    """The requestReceiptTimestamp t2 of two requests that are 1 s apart is 1 s apart."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        first, _ = request_peer_delay(master)
        time.sleep(1)
        second, _ = request_peer_delay(master)

    assert (
        abs(second["timestamp_ns"] - first["timestamp_ns"] - 1_000_000_000)
        < 300_000_000
    )


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req_response",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
@XFAIL_257_NO_PDELAY_FOLLOW_UP_WITHOUT_TX_TIMESTAMP
def test_pdelay_response_follow_up_carries_transmit_timestamp(time_stack):
    """The response origin timestamp t3 is the time the response was sent, shortly after the receipt time t2."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS) as master:
        response, follow_up = request_peer_delay(master)

    t2, t3 = response["timestamp_ns"], follow_up["timestamp_ns"]
    assert t2 > 0, "requestReceiptTimestamp is not set"
    assert 0 <= t3 - t2 < 1_000_000_000, f"t3 {t3} is not shortly after t2 {t2}"


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req_response",
        "comp_req__time_slave__domain_filtering",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@XFAIL_226_PDELAY_REQ_OF_OTHER_DOMAIN_NOT_ANSWERED
def test_pdelay_request_of_other_domain_is_answered(time_stack):
    """The domain number of a received Pdelay_Req is ignored: it is answered whatever the domain is."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, domain=OTHER_DOMAIN) as master:
        request_peer_delay(master, domain=OTHER_DOMAIN)


@add_test_properties(
    partially_verifies=[
        "comp_req__time_slave__pdelay_req_response",
        "comp_req__time_slave__domain_filtering",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@XFAIL_226_PDELAY_MESSAGES_CARRY_CONFIGURED_DOMAIN
def test_pdelay_responses_carry_domain_0_whatever_the_configured_domain(
    target, platform, gptp_iface, other_domain_config
):
    """The Pdelay messages that are sent carry domain 0, also when the time slave is configured for another domain."""
    with GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, domain=OTHER_DOMAIN) as master:
        with running_time_stack(
            target, platform, gptp_iface, config=other_domain_config
        ):
            replies = request_peer_delay(master, domain=OTHER_DOMAIN)

    assert [reply["domain"] for reply in replies] == [0, 0]
