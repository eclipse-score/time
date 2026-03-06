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

.. _inc_time_overview:

########
Overview
########

Purpose
=======

**score_inc_time** is an automotive-grade time synchronization module for
Eclipse S-CORE. It distributes a vehicle-wide synchronized clock to multiple
applications on the same ECU using the gPTP protocol (IEEE 802.1AS) and
lock-free shared memory IPC.

The module consists of two main deliverables:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Component
     - Description
   * - **tsyncd**
     - A daemon that receives gPTP Sync/Follow_Up messages from a Time Master
       over raw Ethernet, computes the clock offset, steps the PTP Hardware
       Clock (PHC), and publishes the synchronized time to a POSIX shared
       memory region.
   * - **libscore_time**
     - A C++ client library that maps the shared memory region and exposes a
       clean, lock-free API for reading vehicle time, absolute (UTC) time, and
       synchronization quality metadata.

Scope
=====

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - In Scope
     - Description
   * - gPTP Slave operation
     - Receives Sync and Follow_Up from a remote Time Master; no Best Master
       Clock Algorithm (BMCA) or Master role.
   * - Peer delay measurement
     - Computes link propagation delay via Pdelay_Req/Resp/Resp_Follow_Up.
   * - Absolute time (UTC)
     - Acquires UTC via NTP or an optional external source (e.g., GNSS via
       Unix socket).
   * - Lock-free IPC
     - Seqlock-based shared memory for zero-overhead multi-reader access.
   * - Linux and QNX
     - Platform shims for raw sockets and PHC on both operating systems.

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Out of Scope
     - Description
   * - gPTP Master / BMCA
     - This module acts as a slave only.
   * - NTP server
     - NTP is consumed, not served.
   * - IEEE 1588 full profile
     - Only the gPTP (802.1AS) automotive profile subset is implemented.

Project Layout
==============

.. code-block:: text

   src/
   ├── libscore_time/          # Client library (public API + mocks)
   ├── common/                 # Shared IPC types and utilities
   └── tsyncd/                 # Synchronization daemon
       ├── daemon/             # Entry point, signal handling
       ├── engine/             # Three-threaded sync engine
       ├── protocol/           # gPTP, Ethernet, NTP protocol layers
       ├── config/             # Configuration file loader
       └── platform/           # Linux / QNX raw-socket shims
   tests/cpp/                  # Google Test unit tests
   docs/                       # This documentation

Quick Start
===========

Build the daemon and client library:

.. code-block:: bash

   bazel build //src/tsyncd:tsyncd //src/libscore_time:score_time

Run the unit-test suite:

.. code-block:: bash

   bazel test //tests/cpp/...

Build the documentation:

.. code-block:: bash

   bazel run //:docs

Client API Example
==================

.. code-block:: cpp

   #include <score_time.hpp>

   auto st = score_time::ScoreTime::Create({});

   // Read synchronized vehicle time
   auto t   = st->VehicleTime().Now();
   auto acc = st->VehicleTime().GetAccuracyQualifier();

   if (acc == score_time::AccuracyQualifier::kSynchronized) {
       // t.count() is nanoseconds since epoch, locked to gPTP master
   }

   // Read absolute UTC time
   auto utc      = st->AbsoluteTime().Now();
   auto inaccNs  = st->AbsoluteTime().GetEstimatedInaccuracyNs();

Configuration
=============

The daemon reads ``tsyncd.conf`` on startup. Key parameters:

.. list-table::
   :header-rows: 1
   :widths: 30 20 50

   * - Parameter
     - Default
     - Description
   * - ``iface_name``
     - ``eth0``
     - Network interface for gPTP frames.
   * - ``phc_device``
     - ``/dev/ptp0``
     - PTP Hardware Clock device (Linux).
   * - ``shm_name``
     - ``/score_time_vehicle_time``
     - POSIX shared memory name.
   * - ``ntp_servers``
     - *(empty)*
     - Comma-separated NTP server list for absolute time.
   * - ``sync_timeout_ms``
     - ``2000``
     - Milliseconds before declaring master loss.
   * - ``unstable_offset_threshold_ns``
     - ``1000``
     - Offset above which sync is considered unstable.
