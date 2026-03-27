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

.. _timeslave_design:

############################
TimeSlave Design
############################

.. contents:: Table of Contents
   :depth: 3
   :local:

Overview
========

**TimeSlave** is a standalone process that implements the gPTP (IEEE 802.1AS) slave endpoint
for the Eclipse SCORE time synchronization system. It receives gPTP Sync/FollowUp messages
from a Time Master on the Ethernet network, measures peer delay, optionally adjusts the PTP
Hardware Clock (PHC), and publishes the resulting ``PtpTimeInfo`` to shared memory for
consumption by the **TimeDaemon**.

TimeSlave is deployed as a separate process from TimeDaemon to isolate the real-time
network I/O (raw socket operations, hardware timestamping) from the higher-level time
validation and distribution logic.

Architecture
============

The TimeSlave process is composed of the following major components:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Component
     - Responsibility
   * - **TimeSlave Application**
     - Lifecycle management (Initialize, Run, Deinitialize). Integrates with ``score::mw::lifecycle``.
   * - **GptpEngine**
     - Core gPTP protocol engine. Manages RxThread and PdelayThread.
   * - **libTSClient (GptpIpcPublisher)**
     - Publishes ``PtpTimeInfo`` to POSIX shared memory using a seqlock protocol.
   * - **PhcAdjuster**
     - Adjusts the PTP Hardware Clock via step or frequency corrections.
   * - **ProbeManager / Recorder**
     - Runtime instrumentation and CSV-based event recording.

Deployment view
---------------

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_deployment.puml
   :alt: TimeSlave Deployment View

.. raw:: html

   </div>

Class view
----------

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_class.puml
   :alt: TimeSlave Class Diagram

.. raw:: html

   </div>

Data flow
---------

The end-to-end data flow from network frame reception to shared memory publication:

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/timeslave_data_flow.puml
   :alt: TimeSlave Data Flow

.. raw:: html

   </div>

Application lifecycle
=====================

The ``TimeSlave`` class extends ``score::mw::lifecycle::Application`` and follows the
standard SCORE lifecycle pattern:

1. **Initialize** — Creates the ``GptpEngine`` with configured options, initializes
   the ``GptpIpcPublisher`` (creates shared memory segment), and prepares the
   ``HighPrecisionLocalSteadyClock`` for local time reference.

2. **Run** — Starts the GptpEngine (which spawns RxThread and PdelayThread internally).
   Enters a periodic loop that:

   - Calls ``GptpEngine::ReadPTPSnapshot()`` to get the latest time measurement
   - Enriches the snapshot with the local clock timestamp
   - Publishes to shared memory via ``GptpIpcPublisher::Publish()``
   - Monitors the ``stop_token`` for graceful shutdown

3. **Deinitialize** — Stops the GptpEngine threads, destroys the shared memory segment.

Platform support
================

TimeSlave supports two target platforms with platform-specific implementations:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Component
     - Linux
     - QNX
   * - Raw Socket
     - ``AF_PACKET`` with ``SO_TIMESTAMPING``
     - QNX raw-socket shim
   * - Network Identity
     - ``ioctl(SIOCGIFHWADDR)``
     - QNX-specific MAC resolution
   * - PHC Adjuster
     - ``clock_adjtime()``
     - EMAC PTP ioctls

Platform selection is handled at compile time via Bazel ``select()`` in the BUILD files.

.. toctree::
   :maxdepth: 2
   :caption: Detailed Design

   gptp_engine/index
   libTSClient/index
