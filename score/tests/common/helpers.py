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

"""Constants and helpers shared by the end-to-end tests."""

import json
import shlex
import statistics
import threading
import time
from contextlib import contextmanager
from dataclasses import dataclass

import pytest

from gptp_grandmaster import (
    MSG_PDELAY_RESP,
    MSG_PDELAY_RESP_FOLLOW_UP,
)

BIN = "/opt/bin"

ENV = "MW_LOG_CONFIG_FILE=/opt/etc/logging.json"

GM_IFACE = "tap0"

# Offset of the grandmaster against the host clock: far enough from the target's
# own clock that a result which merely echoes the local time cannot pass.
GM_OFFSET_NS = 3600 * 1_000_000_000

SYNC_TOLERANCE_NS = 50_000_000

# Largest relative rate difference between the host's CLOCK_MONOTONIC and CLOCK_MONOTONIC_RAW that the tests accept.
# An NTP client slews the clock by at most 500 ppm (0.05 %), so a healthy host stays far below this. The grandmaster
# takes its time from the host's CLOCK_REALTIME, which is slewed together with CLOCK_MONOTONIC, while the clock of a
# KVM guest follows the hardware clock. Over the 3 s that many tests observe, 0.5 % is 15 ms of the 50 ms tolerance.
HOST_CLOCK_MAX_SKEW = 0.005

SYNC_TIMEOUT_S = 30

# Defaults of the time daemon (see comp_req__time_daemon__*): the jump limit is 500 us, jumps are not reported
# for 5 s after the initial synchronization, the timeout is 3.3 s and a jump is cleared by 2 valid frames.
JUMP_THRESHOLD_NS = 500_000

JUMP_DEBOUNCE_S = 5

TIMEOUT_S = 3.3

# Time slave defaults that are configured in config/time_slave_config.json.
PDELAY_WARMUP_S = 2.0

PDELAY_INTERVAL_S = 1.0

# Domain number of the grandmaster in tests that configure the time slave for a domain other than the default 0.
OTHER_DOMAIN = 5


@dataclass(frozen=True)
class Platform:
    # Command that prints the name of the network interface connected to the grandmaster.
    gptp_iface_command: str
    # Shared memory files of the time stack, which are removed before and after each test.
    shm_files: str


# Keyed by the output of `uname -s` on the target.
PLATFORMS = {
    "Linux": Platform(
        gptp_iface_command="ls /sys/class/net | grep -v '^lo$' | head -n 1",
        shm_files="/dev/shm/gptp_* /dev/shm/svt_*",
    ),
    "QNX": Platform(
        gptp_iface_command="echo vtnet0",
        shm_files="/dev/shmem/gptp_* /dev/shmem/svt_*",
    ),
}


def wait_for(condition, timeout_s, what, details=lambda: ""):
    """Poll `condition` until it is true; raise TimeoutError with `details()` appended otherwise."""
    deadline = time.monotonic() + timeout_s
    while True:
        if condition():
            return
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        time.sleep(min(0.05, remaining))
    raise TimeoutError(f"timed out after {timeout_s}s waiting for {what}\n{details()}")


# Smallest deviation of a Pdelay_Req gap that is not treated as an error. The gaps are timestamped by a Python thread on
# the host, so a stall of a loaded CI runner shows up as jitter that the target did not cause.
PDELAY_GAP_MIN_TOLERANCE_S = 0.15


def assert_regular_gaps(gaps, interval_s, relative_tolerance=0.25):
    """The gaps between periodic messages follow `interval_s`, allowing for the jitter of a loaded host.

    Every gap must be within the larger of `relative_tolerance` and PDELAY_GAP_MIN_TOLERANCE_S of the interval. A
    single late message makes one gap longer and the next one shorter by the same amount, so the mean (which cancels
    that) and the median must additionally be within 10 % of the interval.
    """
    tolerance_s = max(relative_tolerance * interval_s, PDELAY_GAP_MIN_TOLERANCE_S)
    assert all(abs(gap - interval_s) <= tolerance_s for gap in gaps), gaps
    assert abs(statistics.mean(gaps) - interval_s) <= 0.1 * interval_s, gaps
    assert abs(statistics.median(gaps) - interval_s) <= 0.1 * interval_s, gaps


def output_of(handle):
    """Output of an asynchronously executed process as text."""
    output = handle.get_output()
    return output.decode(errors="replace") if isinstance(output, bytes) else output


def log_of(handle):
    """Output of a process with whitespace collapsed, as the log pads streamed values with spaces."""
    return " ".join(output_of(handle).split())


def sample_vehicle_time(target, samples=1, interval_ms=100):
    """Run the vehicle time probe on the target and return its parsed samples."""
    exit_code, output = target.execute(
        f"{BIN}/vehicle_time_probe --samples {samples} --interval-ms {interval_ms}"
    )
    assert exit_code == 0, output
    return [
        json.loads(line)
        for line in output.decode().splitlines()
        if line.startswith("{")
    ]


def start_time_slave(
    target, gptp_iface, config="/opt/etc/time_slave_config.json", env=""
):
    config_arg = f" --config {config}" if config else ""
    return target.execute_async(
        f"exec env {ENV} {env} GPTP_IFACE={gptp_iface} {BIN}/time_slave{config_arg}",
        cwd="/opt",
    )


def start_time_daemon(target):
    return target.execute_async(f"exec env {ENV} {BIN}/time_daemon", cwd="/opt")


def remove_shm_files(target, platform):
    target.execute(f"rm -f {platform.shm_files}")


def present_shm_files(target, platform):
    _, output = target.execute(f"ls -1 {platform.shm_files} 2>/dev/null")
    return output.decode().split()


def write_text_file(target, path, text):
    # echo, as QNX has no printf; it adds a newline, which no test depends on.
    exit_code, output = target.execute(f"echo {shlex.quote(text)} > {path}")
    assert exit_code == 0, output


def write_json_file(target, path, content):
    write_text_file(target, path, json.dumps(content))


@contextmanager
def running_time_stack(
    target, platform, gptp_iface, config="/opt/etc/time_slave_config.json", slave_env=""
):
    """Time slave and time daemon running on the target, in this start-up order.

    `config` is passed as --config, unless it is None. `slave_env` holds additional VAR=value settings.
    """
    remove_shm_files(target, platform)
    slave = start_time_slave(target, gptp_iface, config, slave_env)
    try:
        wait_for(
            lambda: "TimeSlave initialized" in output_of(slave),
            10,
            "time_slave to initialize",
            lambda: output_of(slave),
        )
        daemon = start_time_daemon(target)
        try:
            yield slave, daemon
        finally:
            daemon.stop()
    finally:
        slave.stop()
        remove_shm_files(target, platform)


class ProbeStream:
    """Vehicle time probe sampling continuously on the target.

    Polling the probe with one execute() per sample is too slow to see short-lived status changes
    such as a time jump, which is cleared again after two sync frames.

    With `events`, the probe also prints the notifications that it receives, see events().
    """

    def __init__(self, target, interval_ms=20, events=False):
        self._process = target.execute_async(
            f"exec {BIN}/vehicle_time_probe --samples 0 --interval-ms {interval_ms}{' --events' if events else ''}",
            cwd="/opt",
        )
        self._samples = []
        self._events = []
        self._consumed = 0  # characters of the output that are already parsed
        self._fragment = ""  # start of a line whose rest is in the next output line

    @staticmethod
    def _parse_line(line):
        """The JSON object of a sample or event line, or None if the line is not (yet) complete."""
        text = line[len("EVENT") :].lstrip() if line.startswith("EVENT") else line
        if not (text.startswith("{") and text.endswith("}")):
            return None
        try:
            return json.loads(text)
        except json.JSONDecodeError:
            return None

    def _record(self, obj):
        if "event" in obj:
            self._events.append(obj)
        elif "vehicle_time_ns" in obj:
            self._samples.append(obj)

    def _parse_new_output(self):
        # The ITF splits every chunk that it receives from the target into lines on its own, so a line that straddles
        # two chunks arrives as two lines, with whitespace stripped at the cut. Such fragments are joined again. The
        # objects have no nested braces, so a "{" after a fragment begins a new line unless the fragment is "EVENT"
        # with its space stripped, and objects are told apart by their keys rather than by the "EVENT" prefix.
        output = output_of(self._process)
        complete = output.rfind("\n") + 1
        for line in output[self._consumed : complete].splitlines():
            starts_new_line = line.startswith("EVENT") or (
                line.startswith("{") and self._fragment != "EVENT"
            )
            if self._fragment and not starts_new_line:
                candidate = self._fragment + line
            else:
                candidate = line  # a fragment that was not completed is dropped
            obj = self._parse_line(candidate)
            if obj is not None:
                self._record(obj)
                self._fragment = ""
            elif candidate.startswith("{") or "EVENT".startswith(candidate[:5]):
                self._fragment = candidate
            else:
                self._fragment = ""  # other output, e.g. a log line
        self._consumed = complete

    def samples(self):
        """All samples so far. Only the output that is new since the last call is parsed."""
        self._parse_new_output()
        return list(self._samples)

    def events(self, kind=None):
        """All notifications so far (of one `kind`: "status", "sync" or "pdelay"), in the order they were delivered."""
        self._parse_new_output()
        return [e for e in self._events if kind is None or e["event"] == kind]

    def latest(self):
        samples = self.samples()
        return samples[-1] if samples else None

    def stop(self):
        self._process.stop()


@contextmanager
def probing(target, events=False, interval_ms=20):
    stream = ProbeStream(target, interval_ms=interval_ms, events=events)
    try:
        yield stream
    finally:
        stream.stop()


def wait_until_synchronized(target):
    """Block until the vehicle time probe reports reliable time."""
    with probing(target) as stream:
        wait_for_stream_reliable(stream)


def drifts_from_grandmaster_pace_ns(samples):
    """Deviation of each sample's vehicle time from the time elapsed since the first sample, in nanoseconds.

    Vehicle time follows the grandmaster, but the local clock may run at another rate (the clock of a QEMU guest is
    within a few ppm of the host, but that of a host with a slewed clock is not, see the "Host Clock" section of
    the integration test manual). The elapsed local time is therefore divided by the mean rate_deviation, which
    is the local time per grandmaster time. The local time is that of the SteadyClock (CLOCK_MONOTONIC), with which
    the time slave stamps the Syncs to determine the rate deviation; the HighResSteadyClock of Linux is based on
    CLOCK_REALTIME, which the guest slews and steps (see vehicle_time_jumps_ns).
    """
    rate = statistics.mean(s["rate_deviation"] for s in samples)
    assert rate > 0, "no rate deviation reported"
    first = samples[0]
    return [
        (s["vehicle_time_ns"] - first["vehicle_time_ns"])
        - (s["monotonic_time_ns"] - first["monotonic_time_ns"]) / rate
        for s in samples
    ]


def host_clock_skew(window_s=5.0):
    """Relative rate difference of the host's CLOCK_MONOTONIC against its CLOCK_MONOTONIC_RAW over `window_s`.

    CLOCK_MONOTONIC_RAW is not adjusted by NTP or by changes of the tick, CLOCK_MONOTONIC and CLOCK_REALTIME are.
    A positive result means that the host's clocks run fast against the hardware, 0.01 is 1 %. Returns None where
    the host has no CLOCK_MONOTONIC_RAW.
    """
    if not hasattr(time, "CLOCK_MONOTONIC_RAW"):
        return None
    start = (
        time.clock_gettime_ns(time.CLOCK_MONOTONIC),
        time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW),
    )
    time.sleep(window_s)
    end = (
        time.clock_gettime_ns(time.CLOCK_MONOTONIC),
        time.clock_gettime_ns(time.CLOCK_MONOTONIC_RAW),
    )
    return (end[0] - start[0]) / (end[1] - start[1]) - 1


LEAP_FLAGS = ("leap_future", "leap_past")


def wait_for_stream(stream, condition, timeout_s, what):
    """Wait until `condition(latest_sample)` holds for the latest sample of the probe stream."""
    wait_for(
        lambda: (stream.latest() is not None) and condition(stream.latest()),
        timeout_s,
        what,
        lambda: f"latest sample: {stream.latest()}",
    )


def wait_for_stream_reliable(stream):
    wait_for_stream(
        stream,
        lambda s: s["reliable"],
        SYNC_TIMEOUT_S,
        "vehicle time to become reliable",
    )


def wait_for_stream_in_sync(stream, timeout_s=SYNC_TIMEOUT_S):
    """Wait for synchronized time without timeout, whatever the time jump flags are.

    Unlike reliable time, this does not depend on the time jump detection, which flags Syncs whose software
    timestamps jitter by more than its limit (see the note at XFAIL_264_267_JUMP_LIMIT_BELOW_CLOCK_NOISE in
    known_issues), and nearly every Sync when the local clock runs at another rate than the grandmaster.
    """
    wait_for_stream(
        stream,
        lambda s: s["synchronized"] and not s["timeout"],
        timeout_s,
        "vehicle time to be synchronized",
    )


def assert_no_jump_reported(samples):
    assert not [s for s in samples if s["leap_future"] or s["leap_past"]], (
        "unexpected time jump flag"
    )


UNUSABLE_SYNC_DWELL_S = 2.0


# --- peer delay responder (comp_req__time_slave__pdelay_req_response / comp_req__time_slave__ethernet_address / comp_req__time_slave__domain_filtering)


def request_peer_delay(master, domain=None):
    """Send a Pdelay_Req as a peer and return the Pdelay_Resp and Pdelay_Resp_Follow_Up that answer it."""
    sequence_id = master.send_pdelay_request(domain)

    def replies():
        return [
            r for r in list(master.pdelay_replies) if r["sequence_id"] == sequence_id
        ]

    wait_for(
        lambda: len(replies()) >= 2,
        3,
        "the answers to the Pdelay_Req",
        lambda: str(master.pdelay_replies),
    )
    by_type = {r["type"]: r for r in replies()}
    return by_type[MSG_PDELAY_RESP], by_type[MSG_PDELAY_RESP_FOLLOW_UP]


def vehicle_time_jumps_ns(samples):
    """Change of the vehicle time from one sample to the next, beyond the time that the steady clock advanced.

    An update of the vehicle time is applied at once, so it is a single jump between two samples. This does not depend
    on the pace of the vehicle time, which cannot be derived from the steady clock: the HighResSteadyClock of Linux is
    based on CLOCK_REALTIME, which the guest slews and steps.
    """
    return [
        (b["vehicle_time_ns"] - a["vehicle_time_ns"])
        - (b["steady_time_ns"] - a["steady_time_ns"])
        for a, b in zip(samples, samples[1:])
    ]


def wait_for_vehicle_time_jump(
    stream, mark, expected_ns, what, tolerance_ns=SYNC_TOLERANCE_NS
):
    """Wait until the vehicle time jumped by `expected_ns` since the `mark`-th sample, because of `what`."""
    wait_for(
        lambda: any(
            abs(jump - expected_ns) < tolerance_ns
            for jump in vehicle_time_jumps_ns(stream.samples()[mark:])
        ),
        SYNC_TIMEOUT_S,
        f"vehicle time to jump by {expected_ns} ns because of {what}",
        lambda: (
            f"jumps: {[j for j in vehicle_time_jumps_ns(stream.samples()[mark:]) if abs(j) > 10_000_000]}"
        ),
    )


def assert_following_grandmaster(samples):
    """The samples are synchronized, without timeout, and advance at the pace of the grandmaster."""
    assert len(samples) > 10
    assert all(s["synchronized"] and not s["timeout"] for s in samples)
    assert all(
        abs(drift) < SYNC_TOLERANCE_NS
        for drift in drifts_from_grandmaster_pace_ns(samples)
    )


def status_key(status):
    return (
        status["synchronized"],
        status["timeout"],
        status["leap_future"],
        status["leap_past"],
    )


# --- notification of time synchronization updates (feat_req__time__veh_data_notify / feat_req__time__veh_sync_data) ---


FIRST_SEQUENCE_ID = (
    1000  # not 0, which the sequenceId of an update that never happened is
)


def sync_events_after_first_sync(stream):
    """The sync notifications that are for an update of the grandmaster (see the test of the very first one)."""
    return [e for e in stream.events("sync") if e["precise_origin_ns"] != 0]


LEAP_STEP_NS = 100_000_000


# --- latency (feat_req__time__veh_sync_latency / feat_req__time__veh_startup_ready) -----------------------------------

SYNC_INTERVAL_S = 0.125  # the synchronization interval that the grandmaster is specified to use, see GrandMaster


# --- stall detection for latency measurements ----------------------------------------------------------------------
#
# A latency of the order of 100 ms is as small as the stalls of a loaded CI runner (a descheduled vCPU, a starved Python
# thread), and a stall can only make the latency longer. A measurement that was overlapped by a stall therefore says
# nothing about a bound that it exceeds. The tests detect stalls, repeat only the measurements that one overlapped, and
# report "inconclusive" rather than a failure when every attempt was disturbed. A measurement that no stall overlapped
# is trusted, so a real regression still fails at once.

# Overshoot of a sleep, or excess gap between two probe samples, that counts as a stall.
STALL_THRESHOLD_S = 0.03


class HostStallMonitor:
    """Records when the test process on the host was not scheduled for longer than STALL_THRESHOLD_S.

    A thread that sleeps for a short time and checks how late it woke up. It sees a stall of the whole process, which
    also delays the grandmaster's threads that time the Pdelay_Req and Sync messages.
    """

    PERIOD_S = 0.005

    def __init__(self):
        self._stalls = []  # (start, end) in time.monotonic() seconds
        self._running = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def _run(self):
        while self._running.is_set():
            before = time.monotonic()
            time.sleep(self.PERIOD_S)
            after = time.monotonic()
            if after - before - self.PERIOD_S > STALL_THRESHOLD_S:
                self._stalls.append((before, after))

    def __enter__(self):
        self._running.set()
        self._thread.start()
        return self

    def __exit__(self, *exc):
        self._running.clear()
        self._thread.join()

    def stalled_between(self, start, end):
        """Whether a stall overlapped the time.monotonic() interval from `start` to `end`."""
        return any(a < end and b > start for a, b in list(self._stalls))


def probe_stalled_between(samples, start_ns, end_ns, interval_ms=20):
    """Whether two consecutive probe samples around the guest times `start_ns`..`end_ns` are further apart than
    they were asked to be, which means that the guest (or the probe) was not scheduled in between.

    The times are the CLOCK_MONOTONIC values of the guest (`monotonic_time_ns`), and the interval `interval_ms` is the
    sampling period of the probe. This only sees a stall of the probe or of the whole guest, not of one other process.
    """
    limit_ns = interval_ms * 1_000_000 + STALL_THRESHOLD_S * 1e9
    return any(
        b["monotonic_time_ns"] - a["monotonic_time_ns"] > limit_ns
        and a["monotonic_time_ns"] < end_ns
        and b["monotonic_time_ns"] > start_ns
        for a, b in zip(samples, samples[1:])
    )


def measure_latency(trial, limit_s, what, attempts=3):
    """Latency in seconds that is either below `limit_s` or was measured without a stall, from up to `attempts` trials.

    `trial()` performs one complete measurement and returns `(latency_s, stalled)`. A latency that meets the limit is
    accepted even if a stall overlapped it, as a stall cannot shorten it. A longer latency that no stall overlapped is
    returned for the test to fail on. If every attempt was disturbed, the test is skipped as inconclusive.
    """
    results = []
    for _ in range(attempts):
        latency_s, stalled = trial()
        if not stalled or 0 <= latency_s < limit_s:
            return latency_s
        results.append(latency_s)
    pytest.skip(
        f"inconclusive: every attempt to measure {what} was disturbed by a stall of the runner "
        f"(latencies in s: {[round(r, 3) for r in results]})"
    )
