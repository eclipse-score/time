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
"""Minimal IEEE 802.1AS (gPTP) grandmaster for integration tests.

Implements just what the time slave consumes: two-step Sync/Follow_Up towards
the slave and Pdelay_Resp/Pdelay_Resp_Follow_Up in answer to its Pdelay_Req.
Time is software-stamped from CLOCK_REALTIME plus a configurable offset, and
the master can be paused, resumed, stepped or run at another rate, to inject faults.
Further knobs: a link delay that is reported in the Pdelay answers, correction fields,
sequence id increments, Follow_Ups that are late, and Announce/Signaling messages.

Opening the raw socket needs CAP_NET_RAW; run the test with --config=qemu,
which provides it through a private user/network namespace.
"""

import logging
import select
import socket
import sys
import struct
import threading
import time

logger = logging.getLogger(__name__)

ETH_P_1588 = 0x88F7
ETH_P_ALL = 0x0003
ETH_P_8021Q = 0x8100
ETH_P_8021AD = 0x88A8
PTP_MULTICAST_MAC = bytes.fromhex("0180C200000E")
PACKET_OUTGOING = 4

MSG_SYNC = 0x0
MSG_PDELAY_REQ = 0x2
MSG_PDELAY_RESP = 0x3
MSG_FOLLOW_UP = 0x8
MSG_PDELAY_RESP_FOLLOW_UP = 0xA
MSG_ANNOUNCE = 0xB
MSG_SIGNALING = 0xC

SO_TIMESTAMPING = 37
SCM_TIMESTAMPING = 37
SOF_TIMESTAMPING_TX_SOFTWARE = 1 << 1
SOF_TIMESTAMPING_SOFTWARE = 1 << 4

TRANSPORT_SPECIFIC = 0x10
PTP_VERSION = 2
FLAG_TWO_STEP = 0x0200
# The time slave expects the controlField values of IEEE 1588-2008 table 23.
CONTROL = {MSG_SYNC: 0, MSG_FOLLOW_UP: 2}
CONTROL_OTHER = 5
HEADER_LEN = 34
TIMESTAMP_LEN = 10
PORT_IDENTITY_LEN = 10


def _timestamp(ns):
    seconds, nanoseconds = divmod(ns, 1_000_000_000)
    return struct.pack(">HII", seconds >> 32, seconds & 0xFFFFFFFF, nanoseconds)


# Manglers for the `mangle_sync` / `mangle_follow_up` faults: they take the PTP message (without Ethernet header)
# and return the message that is sent instead.
def truncate_to(length):
    return lambda msg: msg[:length]


def set_byte(index, value):
    return lambda msg: msg[:index] + bytes([value]) + msg[index + 1 :]


def set_message_length(length):
    return lambda msg: msg[:2] + struct.pack(">H", length) + msg[4:]


def set_origin_timestamp(seconds, nanoseconds):
    """Replace the 10 octet timestamp that follows the header, which may be out of the range that 802.1AS allows."""
    stamp = struct.pack(">HII", seconds >> 32, seconds & 0xFFFFFFFF, nanoseconds)
    return lambda msg: msg[:HEADER_LEN] + stamp + msg[HEADER_LEN + TIMESTAMP_LEN :]


def header_only(msg):
    """A well-formed message of only the header, i.e. without the body that a Follow_Up needs."""
    return set_message_length(HEADER_LEN)(msg[:HEADER_LEN])


class GrandMaster:
    """gPTP grandmaster bound to one network interface.

    :param iface: interface the time slave is reachable on (e.g. ``tap0``).
    :param offset_ns: master time minus host CLOCK_REALTIME.
    :param sync_interval_s: period of the Sync/Follow_Up messages.
    :param follow_up_sequence_id_offset: if not 0, Follow_Ups do not match their Sync (see corrupt_follow_up_sequence_id).
    :param fault: options of _send_sync_pair() that make the periodic Sync/Follow_Up pairs faulty from the first one on.
    :param first_sequence_id: sequenceId of the first Sync, e.g. close to 0xFFFF to cover the wrap-around.
    :param tx_timestamp_wait_s: how long to wait for the transmit timestamp of a Sync. Without one (0) a Sync is stamped
        with the time before it is sent, which allows a Sync interval below that wait.
    :param answer_pdelay: initial value of the attribute of this name.
    :param pdelay_extra_delay_ns: initial value of the attribute of this name.

    Attributes that can be changed while the master runs:

    * ``sequence_id_increment``: difference between the sequenceIds of two consecutive Syncs (not 1 makes them jump).
    * ``answer_pdelay``: if False, Pdelay_Req messages are received but not answered.
    * ``pdelay_extra_delay_ns``: link delay that is added to what the slave measures with its Pdelay_Req.
    * ``sync_correction_ns`` / ``follow_up_correction_ns``: correctionField of the Sync / Follow_Up messages.
    """

    def __init__(
        self,
        iface,
        offset_ns=0,
        sync_interval_s=0.125,
        domain=0,
        follow_up_sequence_id_offset=0,
        fault=None,
        first_sequence_id=0,
        answer_pdelay=True,
        pdelay_extra_delay_ns=0,
        tx_timestamp_wait_s=0.05,
    ):
        self._iface = iface
        self._tx_timestamp_wait_s = tx_timestamp_wait_s
        self._offset_ns = offset_ns
        # (accumulated, start of the current rate in time.monotonic_ns(), rate in ppm), see set_rate_ppm().
        self._rate_state = (0, 0, 0)
        self.sequence_id_increment = 1
        self.answer_pdelay = answer_pdelay
        self.pdelay_extra_delay_ns = pdelay_extra_delay_ns
        self.sync_correction_ns = 0
        self.follow_up_correction_ns = 0
        self._sync_interval_s = sync_interval_s
        self._domain = domain
        self._log_interval = {0.125: -3, 0.25: -2, 0.5: -1, 1.0: 0}.get(
            sync_interval_s, 0x7F
        )
        self._sequence_id = first_sequence_id & 0xFFFF
        self._pdelay_sequence_id = 0
        # Keyword arguments of _send_sync_pair() used by the periodic sync loop to inject faults.
        self._fault = dict(fault or {})
        if follow_up_sequence_id_offset:
            self.corrupt_follow_up_sequence_id(follow_up_sequence_id_offset)
        self._saved_switch_interval = (
            None  # set while start() has changed the interpreter's switch interval
        )
        self._tx_lock = threading.Lock()
        self._sync_enabled = threading.Event()
        self._sync_enabled.set()
        self._stop = threading.Event()
        self._threads = []
        self._socket = None
        self._tx_socket = None
        self._monitor_socket = None
        self._src_mac = None
        self._clock_identity = None
        self.sync_count = 0
        self.pdelay_count = 0
        # time.monotonic() of every Pdelay_Req of the domain received from the slave
        self.pdelay_request_times = []
        # Pdelay_Resp and Pdelay_Resp_Follow_Up received from the slave, see _record_pdelay_reply()
        self.pdelay_replies = []
        # sequenceId and preciseOriginTimestamp (master time in ns) of every Sync/Follow_Up pair that was sent
        self.sync_log = []
        # sequenceId, requestReceiptTimestamp and responseOriginTimestamp (ns, as put in the messages) of every
        # Pdelay_Req that was answered
        self.pdelay_log = []
        # Every gPTP frame (of any domain) received from the slave: type, destination MAC, ethertype and domain.
        self.received_frames = []
        # Frames of the slave that are not untagged gPTP frames to the gPTP multicast address, see _monitor_loop().
        self.non_conforming_frames = []

    @property
    def port_identity(self):
        """ClockIdentity and port number of the grandmaster, as they are put in its messages."""
        return self._clock_identity + struct.pack(">H", 1)

    @property
    def next_sequence_id(self):
        """sequenceId of the next Sync."""
        with self._tx_lock:
            return self._sequence_id

    @next_sequence_id.setter
    def next_sequence_id(self, sequence_id):
        """Continue the Syncs with `sequence_id`, a jump of the sequenceIds, which the slave does not check."""
        with self._tx_lock:
            self._sequence_id = sequence_id & 0xFFFF

    def send_pdelay_request(self, domain=None):
        """Send a Pdelay_Req as a peer would and return its sequenceId. The replies are collected in pdelay_replies."""
        sequence_id = self._pdelay_sequence_id
        self._pdelay_sequence_id = (sequence_id + 1) & 0xFFFF
        length = HEADER_LEN + TIMESTAMP_LEN + PORT_IDENTITY_LEN
        message = self._header(
            MSG_PDELAY_REQ, length, sequence_id, CONTROL_OTHER, 0x7F, domain=domain
        )
        self._send(message + _timestamp(0) + bytes(PORT_IDENTITY_LEN))
        return sequence_id

    def _adjustment_ns(self):
        """Master time minus host CLOCK_REALTIME: the offset, the steps and the time gained or lost by the rate."""
        accumulated, started_ns, ppm = self._rate_state
        return (
            self._offset_ns
            + accumulated
            + (time.monotonic_ns() - started_ns) * ppm // 1_000_000
        )

    def time_ns(self):
        """Current time of the grandmaster."""
        return time.time_ns() + self._adjustment_ns()

    def step(self, delta_ns):
        """Step the grandmaster time, which the slave sees as a time jump."""
        self._offset_ns += delta_ns

    def set_rate_ppm(self, ppm):
        """Let the grandmaster time run `ppm` parts per million faster than the host clock (negative: slower).

        The time stays continuous; only the pace changes from now on.
        """
        accumulated, started_ns, current_ppm = self._rate_state
        now_ns = time.monotonic_ns()
        # One assignment, so that no thread sees the new start with the old accumulated time.
        self._rate_state = (
            accumulated + (now_ns - started_ns) * current_ppm // 1_000_000,
            now_ns,
            ppm,
        )

    def send_announce(self, domain=None, priority1=0):
        """Send an Announce message that claims to come from the best clock there is. A slave must ignore it."""
        length = HEADER_LEN + TIMESTAMP_LEN + 20
        message = self._header(MSG_ANNOUNCE, length, 0, CONTROL_OTHER, 0, domain=domain)
        message += _timestamp(0) + struct.pack(
            ">hBBBBHB8sHB",
            37,  # currentUtcOffset
            0,
            priority1,
            6,  # gmClockClass
            0x21,  # gmClockAccuracy
            0x4E5D,  # gmOffsetScaledLogVariance
            priority1,  # gmPriority2
            self._clock_identity,
            0,  # stepsRemoved
            0x20,  # timeSource: GPS
        )
        self._send(message)

    def send_signaling(self, domain=None):
        """Send a Signaling message with a message interval request that asks the slave to stop all its messages."""
        tlv = struct.pack(
            ">HH3s3sbbbBH",
            0x0003,  # ORGANIZATION_EXTENSION
            12,
            bytes.fromhex("0080C2"),
            bytes.fromhex("000002"),  # message interval request
            127,  # linkDelayInterval: stop sending Pdelay_Req
            127,  # timeSyncInterval: stop sending Sync
            127,  # announceInterval: stop sending Announce
            3,  # flags
            0,
        )
        length = HEADER_LEN + PORT_IDENTITY_LEN + len(tlv)
        message = self._header(
            MSG_SIGNALING, length, 0, CONTROL_OTHER, 0x7F, domain=domain
        )
        self._send(message + b"\xff" * PORT_IDENTITY_LEN + tlv)

    def pause_sync(self):
        """Stop sending Sync/Follow_Up (Pdelay is still answered)."""
        self._sync_enabled.clear()

    def corrupt_follow_up_sequence_id(self, offset=1):
        """Send Follow_Ups whose sequenceId does not match their Sync (offset 0 restores normal behavior)."""
        if offset == 0:
            self._fault = {}
        else:
            self._fault = {"follow_up_sequence_id_offset": offset}

    def inject_fault(self, **fault):
        """Make the periodic Sync/Follow_Up pairs faulty, see _send_sync_pair() for the options."""
        self._fault = fault

    def resume_sync(self):
        self._sync_enabled.set()

    def send_sync_frames(self, count=1, interval_s=0.2):
        """Send exactly `count` Sync/Follow_Up pairs, `interval_s` apart. Only valid while sync is paused."""
        assert not self._sync_enabled.is_set(), "call pause_sync() first"
        for i in range(count):
            if i:
                time.sleep(interval_s)
            self._send_sync_pair()

    def start(self):
        # Thread switches must not delay the Sync by milliseconds (default switch interval is 5 ms).
        self._saved_switch_interval = sys.getswitchinterval()
        sys.setswitchinterval(0.0002)
        try:
            self._open_sockets()
        except BaseException:
            self._close_sockets()
            raise
        self._threads = [
            threading.Thread(target=self._sync_loop, name="gptp-sync", daemon=True),
            threading.Thread(target=self._rx_loop, name="gptp-rx", daemon=True),
            threading.Thread(
                target=self._monitor_loop, name="gptp-monitor", daemon=True
            ),
        ]
        for thread in self._threads:
            thread.start()
        logger.info("gPTP grandmaster started on %s", self._iface)

    def _open_sockets(self):
        # Sees every frame of the interface, to check how the slave addresses its messages. Opened before the slave
        # sends anything, so that no frame is missed.
        monitor = socket.socket(
            socket.AF_PACKET, socket.SOCK_RAW, socket.htons(ETH_P_ALL)
        )
        self._monitor_socket = monitor
        monitor.bind((self._iface, ETH_P_ALL))
        monitor.settimeout(0.1)
        sock = socket.socket(
            socket.AF_PACKET, socket.SOCK_RAW, socket.htons(ETH_P_1588)
        )
        self._socket = sock
        sock.bind((self._iface, ETH_P_1588))
        sock.settimeout(0.1)
        self._src_mac = sock.getsockname()[4][:6]
        # EUI-48 -> EUI-64 clock identity
        self._clock_identity = self._src_mac[:3] + b"\xff\xfe" + self._src_mac[3:]
        # Separate socket for the Sync messages, whose transmit timestamps are read from its error queue.
        # It is bound with protocol 0 so that it receives no frames, which would otherwise pile up unread.
        tx_sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, 0)
        self._tx_socket = tx_sock
        tx_sock.bind((self._iface, 0))
        tx_sock.setsockopt(
            socket.SOL_SOCKET,
            SO_TIMESTAMPING,
            SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE,
        )

    def _close_sockets(self):
        if self._saved_switch_interval is not None:
            sys.setswitchinterval(self._saved_switch_interval)
            self._saved_switch_interval = None
        for sock in (self._socket, self._tx_socket, self._monitor_socket):
            if sock is not None:
                sock.close()
        self._socket = self._tx_socket = self._monitor_socket = None

    def stop(self):
        self._stop.set()
        for thread in self._threads:
            thread.join(timeout=2)
        self._close_sockets()
        logger.info(
            "gPTP grandmaster stopped (%d Syncs, %d Pdelay exchanges)",
            self.sync_count,
            self.pdelay_count,
        )

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *exc):
        self.stop()

    def _header(
        self,
        msg_type,
        length,
        sequence_id,
        control,
        log_interval,
        correction_ns=0,
        domain=None,
    ):
        return struct.pack(
            ">BBHBBHqI8sHHBb",
            TRANSPORT_SPECIFIC | msg_type,
            PTP_VERSION,
            length,
            self._domain if domain is None else domain,
            0,
            FLAG_TWO_STEP,
            correction_ns << 16,  # correctionField is in scaled nanoseconds (2^-16 ns)
            0,
            self._clock_identity,
            1,  # portNumber
            sequence_id,
            control,
            log_interval,
        )

    def _send(self, payload):
        self._socket.send(
            PTP_MULTICAST_MAC + self._src_mac + struct.pack(">H", ETH_P_1588) + payload
        )

    def _send_sync_pair(
        self,
        follow_up_sequence_id_offset=0,
        send_follow_up=True,
        mangle_sync=None,
        mangle_follow_up=None,
        follow_up_delay_s=0.0,
    ):
        """Send one Sync and (optionally) its Follow_Up.

        :param follow_up_sequence_id_offset: added to the sequenceId of the Follow_Up to make it not match its Sync.
        :param mangle_sync: function that turns the PTP message of the Sync into the message that is sent.
        :param mangle_follow_up: same for the Follow_Up.
        :param follow_up_delay_s: time between the Sync and its Follow_Up.
        """
        with self._tx_lock:
            sequence_id = self._sequence_id
            self._sequence_id = (sequence_id + self.sequence_id_increment) & 0xFFFF
            sync = self._header(
                MSG_SYNC,
                HEADER_LEN + TIMESTAMP_LEN,
                sequence_id,
                CONTROL[MSG_SYNC],
                self._log_interval,
                correction_ns=self.sync_correction_ns,
            )
            fup_seq = (sequence_id + follow_up_sequence_id_offset) & 0xFFFF
            fup = self._header(
                MSG_FOLLOW_UP,
                HEADER_LEN + TIMESTAMP_LEN,
                fup_seq,
                CONTROL[MSG_FOLLOW_UP],
                self._log_interval,
                correction_ns=self.follow_up_correction_ns,
            )
            sync_message = sync + _timestamp(0)
            if mangle_sync:
                sync_message = mangle_sync(sync_message)
            sync_frame = (
                PTP_MULTICAST_MAC
                + self._src_mac
                + struct.pack(">H", ETH_P_1588)
                + sync_message
            )
            t_origin = self._send_with_tx_timestamp(sync_frame)
            self.sync_log.append(
                {
                    "sequence_id": sequence_id,
                    "origin_ns": t_origin,
                    "monotonic": time.monotonic(),
                }
            )
            if send_follow_up:
                if follow_up_delay_s:
                    self._stop.wait(follow_up_delay_s)
                fup_message = fup + _timestamp(t_origin)
                if mangle_follow_up:
                    fup_message = mangle_follow_up(fup_message)
                self._send(fup_message)
            self.sync_count += 1

    def _send_with_tx_timestamp(self, frame):
        """Send `frame` and return the grandmaster time at which it left the host.

        A real grandmaster puts the transmit timestamp of the Sync into the Follow_Up. Reading the clock before
        sending would add the scheduling jitter of this process to the time the slave sees.
        """
        self._discard_stale_tx_timestamps()
        before = self.time_ns()
        self._tx_socket.send(frame)
        if not self._tx_timestamp_wait_s:
            return before
        try:
            ready = select.select([], [], [self._tx_socket], self._tx_timestamp_wait_s)[
                2
            ]
            if ready:
                _, ancillary, _, _ = self._tx_socket.recvmsg(
                    2048, 1024, socket.MSG_ERRQUEUE
                )
                for level, kind, data in ancillary:
                    if (
                        level == socket.SOL_SOCKET
                        and kind == SCM_TIMESTAMPING
                        and len(data) >= 16
                    ):
                        seconds, nanoseconds = struct.unpack(
                            "qq", data[:16]
                        )  # software timestamp
                        if seconds:
                            return (
                                seconds * 1_000_000_000
                                + nanoseconds
                                + self._adjustment_ns()
                            )
        except OSError:
            pass
        return before

    def _discard_stale_tx_timestamps(self):
        """Empty the error queue, so that a timestamp left over from an earlier Sync is not mistaken for this one."""
        try:
            while True:
                self._tx_socket.recvmsg(
                    2048, 1024, socket.MSG_ERRQUEUE | socket.MSG_DONTWAIT
                )
        except OSError:
            pass

    def _sync_loop(self):
        next_send = time.monotonic()
        while not self._stop.wait(max(0.0, next_send - time.monotonic())):
            next_send += self._sync_interval_s
            if not self._sync_enabled.is_set():
                continue
            self._send_sync_pair(**self._fault)

    def _rx_loop(self):
        while not self._stop.is_set():
            try:
                frame, addr = self._socket.recvfrom(2048)
            except socket.timeout:
                continue
            t_receipt = self.time_ns()
            t_monotonic = time.monotonic()
            if addr[2] == PACKET_OUTGOING or len(frame) < 14 + HEADER_LEN:
                continue
            payload = frame[14:]
            self.received_frames.append(
                {
                    "type": payload[0] & 0x0F,
                    "destination": frame[:6],
                    "ethertype": struct.unpack(">H", frame[12:14])[0],
                    "domain": payload[4],
                }
            )
            if payload[0] & 0x0F == MSG_PDELAY_REQ and payload[4] == self._domain:
                self.pdelay_request_times.append(t_monotonic)
                self._answer_pdelay_req(payload, t_receipt)
            elif payload[0] & 0x0F in (MSG_PDELAY_RESP, MSG_PDELAY_RESP_FOLLOW_UP):
                self._record_pdelay_reply(frame)

    def _monitor_loop(self):
        while not self._stop.is_set():
            try:
                frame, addr = self._monitor_socket.recvfrom(2048)
            except socket.timeout:
                continue
            if (
                addr[2] == PACKET_OUTGOING
                or len(frame) < 14
                or frame[6:12] == self._src_mac
            ):
                continue
            destination = frame[:6]
            (ethertype,) = struct.unpack(">H", frame[12:14])
            tagged = ethertype in (ETH_P_8021Q, ETH_P_8021AD)
            if tagged or (destination == PTP_MULTICAST_MAC and ethertype != ETH_P_1588):
                self.non_conforming_frames.append(
                    {"destination": destination, "ethertype": ethertype}
                )

    def _record_pdelay_reply(self, frame):
        payload = frame[14:]
        if len(payload) < HEADER_LEN + TIMESTAMP_LEN + PORT_IDENTITY_LEN:
            return
        high, low, nanoseconds = struct.unpack(
            ">HII", payload[HEADER_LEN : HEADER_LEN + TIMESTAMP_LEN]
        )
        self.pdelay_replies.append(
            {
                "type": payload[0] & 0x0F,
                "destination": frame[:6],
                "domain": payload[4],
                "sequence_id": struct.unpack(">H", payload[30:32])[0],
                "timestamp_ns": ((high << 32 | low) * 1_000_000_000) + nanoseconds,
                "requesting_port": payload[HEADER_LEN + TIMESTAMP_LEN :][
                    :PORT_IDENTITY_LEN
                ],
            }
        )

    def _answer_pdelay_req(self, req, t_receipt):
        if not self.answer_pdelay:
            return
        sequence_id = struct.unpack(">H", req[30:32])[0]
        requesting_port = req[20 : 20 + PORT_IDENTITY_LEN]
        length = HEADER_LEN + TIMESTAMP_LEN + PORT_IDENTITY_LEN
        resp = self._header(MSG_PDELAY_RESP, length, sequence_id, CONTROL_OTHER, 0x7F)
        # t2 is only used in (t2 - t1) + (t4 - t3), so adding 2 * D to it makes the slave measure D more link delay.
        resp += _timestamp(t_receipt + 2 * self.pdelay_extra_delay_ns) + requesting_port
        fup = self._header(
            MSG_PDELAY_RESP_FOLLOW_UP, length, sequence_id, CONTROL_OTHER, 0x7F
        )
        t_response = self.time_ns()
        self.pdelay_log.append(
            {
                "sequence_id": sequence_id,
                "request_receipt_ns": t_receipt + 2 * self.pdelay_extra_delay_ns,
                "response_origin_ns": t_response,
            }
        )
        fup += _timestamp(t_response) + requesting_port
        self._send(resp)
        self._send(fup)
        self.pdelay_count += 1
