..
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

.. _gptp_engine_design:

############################
gPTP Engine Design
############################

.. contents:: Table of Contents
   :depth: 3
   :local:

Overview
========

The ``GptpEngine`` is the core protocol engine of TimeSlave. It implements the IEEE 802.1AS
gPTP slave clock functionality by managing two dedicated threads for network I/O and peer
delay measurement.

Class view
==========

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_engine_class.puml
   :alt: gPTP Engine Class Diagram

.. raw:: html

   </div>

Threading model
===============

The GptpEngine operates with two background threads:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/gptp_threading.puml
   :alt: gPTP Engine Threading Model

.. raw:: html

   </div>

RxThread
--------

The RxThread is the primary receive path. It runs a continuous loop:

1. **Recv** — Blocks on ``IRawSocket::Recv()`` with a configurable timeout, receiving raw
   Ethernet frames with hardware timestamps from the NIC.

2. **Decode** — ``FrameCodec::ParseEthernetHeader()`` strips the Ethernet header (with VLAN
   support) and validates the EtherType (``0x88F7``).

3. **Parse** — ``MessageParser::Parse()`` decodes the PTP payload into a ``PTPMessage``
   structure, identifying the message type (Sync, FollowUp, PdelayResp, PdelayRespFollowUp).

4. **Dispatch** — Based on message type:

   - **Sync** → ``SyncStateMachine::OnSync()`` stores the Sync timestamp
   - **FollowUp** → ``SyncStateMachine::OnFollowUp()`` correlates with the preceding Sync,
     computes ``offset_ns`` and ``neighborRateRatio``, and detects time jumps
   - **PdelayResp** → ``PeerDelayMeasurer::OnResponse()``
   - **PdelayRespFollowUp** → ``PeerDelayMeasurer::OnResponseFollowUp()``

5. **Snapshot** — After processing, the latest ``PtpTimeInfo`` snapshot is updated under
   mutex protection.

PdelayThread
------------

The PdelayThread performs IEEE 802.1AS peer delay measurement on a periodic interval
(configurable via ``GptpEngineOptions::pdelay_interval_ms``):

1. **Send** — ``PeerDelayMeasurer::SendRequest()`` transmits a ``PDelayReq`` frame via the
   raw socket, capturing the hardware transmit timestamp (``t1``).

2. **Receive** — The RxThread dispatches incoming ``PDelayResp`` (providing ``t2``) and
   ``PDelayRespFollowUp`` (providing ``t3c``) to the ``PeerDelayMeasurer``.

3. **Compute** — The peer delay is computed using the IEEE 802.1AS formula:

   .. code-block:: text

      path_delay = ((t2 - t1) + (t4 - t3c)) / 2

   where ``t4`` is the local hardware receive timestamp of the PDelayResp frame.

Thread safety is ensured via a mutex in ``PeerDelayMeasurer``, as ``SendRequest()`` runs on
the PdelayThread while ``OnResponse()``/``OnResponseFollowUp()`` are called from the
RxThread.

Core components
===============

FrameCodec
----------

Handles raw Ethernet frame encoding and decoding:

- ``ParseEthernetHeader()`` — Parses source/destination MAC, handles 802.1Q VLAN tags,
  extracts EtherType and payload offset.
- ``AddEthernetHeader()`` — Constructs Ethernet headers for outgoing PDelayReq frames using
  the standard PTP multicast destination MAC (``01:80:C2:00:00:0E``).

MessageParser
-------------

Parses the PTP wire format (IEEE 1588-v2) from raw payload bytes:

- Validates the PTP header (version, domain, message length).
- Decodes message-type-specific bodies: ``SyncBody``, ``FollowUpBody``, ``PdelayReqBody``,
  ``PdelayRespBody``, ``PdelayRespFollowUpBody``.
- All wire structures are packed (``__attribute__((packed))``) for direct memory mapping.

SyncStateMachine
----------------

Implements the two-step Sync/FollowUp correlation logic:

- **OnSync(msg)** — Stores the Sync message and its hardware receive timestamp.
- **OnFollowUp(msg)** — Matches with the preceding Sync by sequence ID, then computes:

  - ``offset_ns`` = master_time - slave_receive_time - path_delay
  - ``neighborRateRatio`` from successive Sync intervals (master vs. slave clock progression)
  - Time jump detection (forward/backward) against configurable thresholds

- **Timeout detection** — Uses ``std::atomic<bool>`` for thread-safe timeout status,
  set when no Sync is received within ``sync_timeout_ms``.

PeerDelayMeasurer
-----------------

Implements the IEEE 802.1AS two-step peer delay measurement protocol:

- Manages the four timestamps (``t1``, ``t2``, ``t3c``, ``t4``) across two threads.
- ``SendRequest()`` — Builds and sends a PDelayReq frame, records ``t1`` from the
  hardware transmit timestamp.
- ``OnResponse()`` / ``OnResponseFollowUp()`` — Records ``t2``, ``t3c``, ``t4`` and
  computes the path delay when all timestamps are available.
- Returns ``PDelayResult`` with ``path_delay_ns`` and a ``valid`` flag.

PhcAdjuster
-----------

Synchronizes the PTP Hardware Clock (PHC) on the NIC:

- **Step correction** — For large offsets exceeding ``step_threshold_ns``, applies an
  immediate time step to the PHC.
- **Frequency slew** — For smaller offsets, adjusts the clock frequency (in ppb) for
  smooth convergence.
- Platform-specific: Linux uses ``clock_adjtime()``, QNX uses EMAC PTP ioctls.
- Configured via ``PhcConfig`` (device path, step threshold, enable/disable flag).

Instrumentation
===============

ProbeManager
------------

A singleton that records probe events at key processing points. Probe points include:

- ``RxPacketReceived`` — Raw frame received from socket
- ``SyncFrameParsed`` — Sync message successfully parsed
- ``FollowUpProcessed`` — Offset computed from Sync/FollowUp pair
- ``OffsetComputed`` — Final offset value available
- ``PdelayReqSent`` — PDelayReq frame transmitted
- ``PdelayCompleted`` — Peer delay measurement completed
- ``PhcAdjusted`` — PHC adjustment applied

The ``GPTP_PROBE()`` macro provides zero-overhead when probing is disabled.

Recorder
--------

Thread-safe CSV file writer that persists probe events and other diagnostic data. Each
``RecordEntry`` contains a timestamp, event type, offset, peer delay, sequence ID, and
status flags.

Configuration
=============

The ``GptpEngineOptions`` struct provides all configurable parameters:

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Parameter
     - Type
     - Description
   * - ``interface_name``
     - string
     - Network interface for gPTP frames (e.g., ``eth0``)
   * - ``pdelay_interval_ms``
     - uint32_t
     - Interval between PDelayReq transmissions
   * - ``sync_timeout_ms``
     - uint32_t
     - Timeout for Sync message reception before declaring timeout state
   * - ``time_jump_forward_ns``
     - int64_t
     - Threshold for forward time jump detection
   * - ``time_jump_backward_ns``
     - int64_t
     - Threshold for backward time jump detection
   * - ``phc_config``
     - PhcConfig
     - PHC device path, step threshold, and enable flag
