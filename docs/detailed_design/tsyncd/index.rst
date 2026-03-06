..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

.. _inc_time_dd_tsyncd:

######
tsyncd
######

Overview
========

``tsyncd`` is a single-process daemon with three worker threads. It is
configured via ``tsyncd.conf``, initialises a ``TimeSyncEngine``, and blocks
on ``SIGTERM`` / ``SIGINT`` for graceful shutdown.

Entry Point
===========

``src/tsyncd/daemon/main.cpp`` parses the ``--config`` command-line argument
(defaults to ``./tsyncd.conf``), calls ``ConfigLoader::Load()`` to populate
``EngineOptions``, constructs a ``TimeSyncEngine``, and waits for a signal.

Configuration
=============

``ConfigLoader`` (``src/tsyncd/config/config_loader.hpp/cpp``) reads an
INI-style key=value file. All parsing is exception-free using
``StringParser::ParseInteger`` and ``StringParser::ParseDouble`` which are
backed by ``std::from_chars`` and ``strtod`` respectively.

Key ``EngineOptions`` fields:

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Field
     - Default
     - Description
   * - ``iface_name``
     - ``eth0``
     - Network interface for gPTP raw socket.
   * - ``phc_device``
     - ``/dev/ptp0``
     - PTP Hardware Clock device (Linux only).
   * - ``shm_name``
     - ``/score_time_vehicle_time``
     - POSIX shared memory name.
   * - ``shm_size``
     - 4096
     - Shared memory region size in bytes.
   * - ``ntp_servers``
     - *(empty)*
     - Comma-separated list of NTP server hostnames.
   * - ``ntp_query_interval_ms``
     - 1000
     - Interval between NTP queries.
   * - ``ntp_samples_to_lock``
     - 5
     - Minimum valid samples before abs time is published.
   * - ``sync_timeout_ms``
     - 2000
     - Milliseconds without Sync before master-loss is declared.
   * - ``pdelay_timeout_ms``
     - 4000
     - Milliseconds without Pdelay response before link-down.
   * - ``unstable_offset_threshold_ns``
     - 1000
     - Offset above which sync state is set to ``kUnstable``.
   * - ``jump_future_threshold_ns``
     - 1 000 000 000
     - Offset above which ``kTimeJumpDetected`` is set.

TimeSyncEngine
==============

``src/tsyncd/engine/time_sync_engine.hpp/cpp`` is the central coordination
class. On construction it:

1. Creates the ``ShmRegion`` and initialises ``SharedState`` (magic,
   version, size fields).
2. Opens the platform raw socket on ``iface_name``.
3. Spawns ``RxLoop``, ``PdelayLoop``, and ``AbsLoop`` as ``std::thread``
   objects.

PortSynchronize()
-----------------

Called by ``RxLoop`` after every valid Sync/Follow_Up pair:

1. Calls the platform shim to step the PHC by the computed offset.
2. Determines the new ``AccuracyQualifier``:

   - ``|offset| > jump_future_threshold_ns`` → ``kTimeJumpDetected``
   - ``|offset| > unstable_offset_threshold_ns`` → ``kUnstable``
   - otherwise → ``kSynchronized``

3. Sets ``TimePointQualifier`` to ``kASIL_B`` when synchronized.
4. Calls ``WriteVehicle()`` to publish all fields to shared memory.
5. Calls ``LogVehicle()`` to append a ``kVehicleOffset`` event.

RxLoop — Sync/Follow_Up State Machine
--------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 15 25 60

   * - State
     - Trigger
     - Action
   * - ``kEmpty``
     - ``SYNC`` received
     - Store ``(sequenceId, t_rx_hw)`` → go to ``kHaveSync``
   * - ``kHaveSync``
     - ``FOLLOW_UP`` with matching ``sequenceId``
     - Compute ``offset = t_rx_hw − (fup_ts + corr_sync + corr_fup)``
       → call ``PortSynchronize()`` → go to ``kEmpty``
   * - ``kHaveSync``
     - ``SYNC`` with different ``sequenceId``
     - Overwrite stored state → stay in ``kHaveSync``
   * - any
     - ``PDELAY_RESP`` / ``PDELAY_RESP_FUP``
     - Deliver to ``PdelayLoop`` via mutex-protected struct

PdelayLoop — Peer Delay Computation
-------------------------------------

Timestamps involved:

.. list-table::
   :header-rows: 1
   :widths: 10 90

   * - Symbol
     - Meaning
   * - ``t1``
     - Hardware Tx timestamp of ``Pdelay_Req`` (captured by shim)
   * - ``t2``
     - Master's hardware Rx timestamp (carried in ``Pdelay_Resp``)
   * - ``t3``
     - Master's precise Tx timestamp (carried in ``Pdelay_Resp_FUP``)
   * - ``t4``
     - Hardware Rx timestamp of ``Pdelay_Resp`` (captured by shim)

Formula: ``path_delay = ((t2 − t1) + (t4 − t3_corrected)) / 2``

AbsLoop — Absolute Time and Health Monitor
-------------------------------------------

The AbsLoop runs two independent sub-tasks:

1. **NTP queries** — fires every ``ntp_query_interval_ms``. Each response
   updates an EWMA estimator for offset and jitter. After
   ``ntp_samples_to_lock`` successful responses the abs time is published
   with ``kTrustworthy`` security qualifier. NTP source identifier is ``2``.

2. **External source** — if ``abs_external_enable`` is set, the loop reads
   from a Unix domain socket at ``abs_source_socket``. Messages older than
   ``abs_source_timeout_ms`` are ignored. When a valid message arrives the
   external source (identifier ``1``) is preferred over NTP.

3. **Timeout monitoring** — on each tick the loop checks:

   - ``sync_timeout_ms`` without a ``PortSynchronize()`` call → publishes
     ``AccuracyQualifier::kNotSynchronized`` and logs ``kVehicleState``.
   - ``pdelay_timeout_ms`` without a ``PdelayLoop`` response → logs
     ``kVehiclePeerDelay`` with a sentinel value.

gPTP Message Parsing
====================

``src/tsyncd/protocol/gptp_protocol.hpp/cpp`` provides
``parse_gptp_message()``. All multi-byte fields are read with explicit
``ntohs`` / ``ntohl`` / ``be64toh`` conversions to handle the big-endian
wire format on little-endian hosts.

The ``PTPHeader`` struct layout (packed):

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Field
     - Size
     - Notes
   * - ``tsmt``
     - 1 byte
     - Lower nibble = message type (0x0 = Sync, 0x8 = FUP, 0x2/0x3/0xA = Pdelay)
   * - ``version``
     - 1 byte
     - Must be 2 for gPTP
   * - ``messageLength``
     - 2 bytes BE
     - Total PTP PDU length
   * - ``correctionField``
     - 8 bytes BE signed
     - Cumulative correction in nanoseconds × 2¹⁶
   * - ``sourcePortIdentity``
     - 10 bytes
     - ClockIdentity (8 bytes) + portNumber (2 bytes)
   * - ``sequenceId``
     - 2 bytes BE
     - Used to match Sync ↔ Follow_Up, Pdelay_Req ↔ Resp

Clock Identity
==============

``src/tsyncd/protocol/net_identity.hpp/cpp`` derives the IEEE 1588
``ClockIdentity`` from the interface MAC address by inserting ``0xFF 0xFE``
at bytes 3–4 (EUI-64 format). This identity is placed in the
``sourcePortIdentity`` of all transmitted PTP frames.
