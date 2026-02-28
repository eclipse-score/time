..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

SCORE Time Synchronization Documentation
==========================================

This documentation describes the SCORE (Scalable Core for Automotive Real-time Environments) Time Synchronization system, which provides high-precision time synchronization for automotive applications using gPTP (IEEE 802.1AS) and NTP.

.. contents:: Table of Contents
   :depth: 2
   :local:

Overview
--------

SCORE Time Synchronization is a safety-critical time synchronization daemon (**tsyncd**) and client library (**libscore_time**) designed for automotive systems. It provides:

- **Vehicle Time**: High-precision synchronized time using gPTP (IEEE 802.1AS) over Ethernet
- **Absolute Time**: UTC/wall-clock time from GNSS, NTP, or kernel RTC
- **Lock-free IPC**: Shared memory communication using seqlock protocol for zero-copy reads
- **AUTOSAR Compliance**: Designed for ASIL QM level with safety qualifiers
- **Multi-platform**: Supports Linux and QNX operating systems

Key Features
------------

**Time Synchronization**

- gPTP (IEEE 802.1AS) slave implementation for vehicle time
- NTP client for absolute time synchronization
- Support for external time sources (GNSS)
- Automatic fallback and error recovery

**Performance & Safety**

- Lock-free shared memory IPC using seqlock protocol
- Zero-copy time reads for clients
- ASIL QM compliance with safety qualifiers
- Sub-microsecond precision for vehicle time

**Platform Support**

- Linux with PTP Hardware Clock (PHC) support
- QNX with BPF packet capture
- Bazel build system for reproducible builds

Project Layout
--------------

The SCORE Time Synchronization project includes the following structure:

- `src/common/`: Shared IPC and utility code
  - `include/score_time/ipc/`: Shared memory structures (SharedState, ShmRegion)
  - `include/score_time/utils/`: Utilities (PthreadLockGuard, time functions, parsers)
- `src/tsyncd/`: Time synchronization daemon implementation
  - `engine/`: Core synchronization engine and state machine
  - `platform/`: Platform-specific code (Linux, QNX)
  - `protocol/`: gPTP and NTP protocol implementations
- `src/libscore_time/`: Client library for reading synchronized time
- `tests/`: Unit and integration tests
- `docs/`: API documentation and architecture diagrams

Quick Start
-----------

**Building the Project**

To build all components:

.. code-block:: bash

   # Build all components
   bazel build //src/...

   # Build tsyncd daemon only
   bazel build //src/tsyncd:tsyncd

   # Build client library
   bazel build //src/libscore_time:score_time

   # Run all tests
   bazel test //tests/...

**Running tsyncd Daemon**

.. code-block:: bash

   # Run with default configuration
   sudo bazel run //src/tsyncd:tsyncd -- --iface eth0

   # Run with custom configuration file
   sudo bazel run //src/tsyncd:tsyncd -- --config /path/to/config.ini

**Using Client Library**

.. code-block:: cpp

   #include "score_time/ipc/shared_state.hpp"
   #include "score_time/ipc/shm_region.hpp"

   // Open shared memory
   score_time::ipc::ShmRegion shm;
   shm.Open("/score_time_vehicle_time", sizeof(score_time::ipc::SharedState), false);

   auto* state = static_cast<score_time::ipc::SharedState*>(shm.Addr());

   // Read vehicle time
   std::int64_t base_ns, mono_ns;
   score_time::AccuracyQualifier acc;
   score_time::TimePointQualifier tpq;

   if (score_time::ipc::ReadVehicle(*state, base_ns, mono_ns, acc, tpq)) {
       // Use synchronized time
   }

Configuration
-------------

The daemon can be configured via command-line arguments or a configuration file (`config.ini`).

**Key Configuration Parameters:**

- `iface_name`: Network interface for gPTP (e.g., "eth0")
- `phc_device`: PTP Hardware Clock device (e.g., "/dev/ptp0")
- `shm_name`: Shared memory object name (default: "/score_time_vehicle_time")
- `ntp_servers`: List of NTP servers for absolute time
- `abs_mode`: Publish-only or discipline system clock

See `docs/configuration.md` for full configuration options.


API Reference
-------------

.. toctree::
   :maxdepth: 2

   api/index

Architecture Diagrams
---------------------

.. toctree::
   :maxdepth: 1

   diagrams/index

