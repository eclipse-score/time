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

.. _inc_time_architecture:

############
Architecture
############

High-Level Architecture
=======================

score_inc_time follows a single-producer / multi-consumer architecture:

- **tsyncd** is the sole producer. It runs as a privileged daemon, opens a
  raw Ethernet socket, receives gPTP frames from a network Time Master, and
  writes synchronized time into a POSIX shared memory region.
- **libscore_time** is the consumer library. Any number of client processes
  map the same shared memory region and read time values with zero system-call
  overhead via the lock-free seqlock protocol.

.. uml:: _assets/system_architecture.puml
   :caption: score_inc_time high-level system architecture

Components
==========

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Component
     - Responsibility
   * - **tsyncd**
     - Daemon process. Receives gPTP Sync/Follow_Up over raw Ethernet,
       computes clock offset, adjusts PHC, and publishes vehicle and absolute
       time to shared memory.
   * - **libscore_time**
     - Client library. Maps shared memory and exposes ``IVehicleTime``,
       ``IAbsoluteTime``, ``IHighPrecisionClock``, and ``IMonotonicClock``
       interfaces.
   * - **common/ipc**
     - Shared header-only library defining ``SharedState``, seqlock
       read/write functions, ``ShmRegion`` RAII wrapper, and utility types.
   * - **platform shims**
     - ``linux_raw_shim`` / ``qnx_raw_shim`` — isolate all OS-specific raw
       socket and PHC operations behind a uniform interface.

Thread Architecture
===================

The ``TimeSyncEngine`` spawns three worker threads at startup:

.. uml:: _assets/thread_architecture.puml
   :caption: tsyncd thread model and state machines

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Thread
     - Description
   * - **RxLoop**
     - Blocks on the raw socket. For each frame it parses the Ethernet and
       gPTP headers, then drives a two-state machine:

       - ``kEmpty`` → receives ``Sync`` → stores ``(sequenceId, t_rx_hw)`` → ``kHaveSync``
       - ``kHaveSync`` → receives matching ``Follow_Up`` → computes
         ``offset = t_rx_hw − (t_fup_precise + corr_sync + corr_fup)`` →
         calls ``PortSynchronize()`` → ``kEmpty``

       ``Pdelay_Resp`` / ``Pdelay_Resp_Follow_Up`` frames are forwarded to
       the PdelayLoop state machine via a shared mutex-protected structure.

   * - **PdelayLoop**
     - Fires on a configurable interval (default 1 s). Sends a
       ``Pdelay_Req`` frame (capturing Tx hardware timestamp ``t1``),
       waits for ``Pdelay_Resp`` (``t2``, ``t4``) and
       ``Pdelay_Resp_Follow_Up`` (precise ``t3``), then computes:

       ``path_delay = ((t2 − t1) + (t4 − t3_corrected)) / 2``

       The result is stored in ``vehicle_path_delay_ns`` of shared memory.

   * - **AbsLoop**
     - Queries NTP servers on a background ticker, applies EWMA smoothing
       to the offset and jitter estimates, and optionally consumes an
       external UTC source (e.g., GNSS) from a Unix domain socket.
       Also monitors sync and peer delay timeouts and degrades
       ``AccuracyQualifier`` accordingly.

Shared Memory IPC
=================

.. uml:: _assets/shared_memory_layout.puml
   :caption: SharedState memory layout and type relationships

The shared memory region is structured as a ``SharedState`` object placed at
the start of a POSIX ``shm_open`` mapping. It contains:

- A **header** with magic (``0x53434F52``), ABI version, and region size for
  reader-side validation.
- A **vehicle time block** — base nanosecond timestamp, monotonic reference,
  accuracy and ASIL qualifiers, last offset, path delay — all protected by
  a single 64-bit seqlock counter.
- An **absolute time block** — UTC base, inaccuracy estimate, source
  identifier, security qualifier — protected by its own seqlock counter.
- Two **circular event logs** (256 entries each) with per-entry seqlocks for
  field-level consistency on weakly-ordered architectures.

**Seqlock write (tsyncd — single writer):**

.. code-block:: cpp

   vehicle_seq.fetch_add(1, memory_order_acq_rel);  // odd → writing
   /* store all payload fields with memory_order_release */
   vehicle_seq.fetch_add(1, memory_order_acq_rel);  // even → done

**Seqlock read (libscore_time — N concurrent readers):**

.. code-block:: cpp

   for (int i = 0; i < 1000; ++i) {
       auto a = vehicle_seq.load(memory_order_acquire);
       if (a & 1) continue;      // odd → write in progress
       /* load all payload fields */
       auto b = vehicle_seq.load(memory_order_acquire);
       if (a == b) return true;  // consistent snapshot
   }
   return false;                 // retry limit exceeded (very rare)

Key properties:

- Readers never block the writer.
- The retry limit (1 000) bounds execution time on safety-critical paths.
- No OS mutex, no system call on the read path.

Protocol Stack
==============

.. uml:: _assets/protocol_stack.puml
   :caption: gPTP protocol stack and layer responsibilities

The gPTP implementation is layered as follows:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Layer
     - File / Responsibility
   * - **Application**
     - ``time_sync_engine.cpp`` — drives the Sync/Follow_Up and peer delay
       state machines; calls into the protocol layer to send/receive frames.
   * - **gPTP (IEEE 802.1AS)**
     - ``gptp_protocol.hpp/cpp`` — parses and serialises PTP headers,
       timestamps (48-bit seconds + 32-bit nanoseconds), and 64-bit
       correction fields.
   * - **Ethernet (IEEE 802.3)**
     - ``eth_protocol.hpp/cpp`` — assembles and strips Ethernet frames
       (EtherType ``0x88F7``), handles optional 802.1Q VLAN tags.
   * - **Platform shim**
     - ``linux_raw_shim`` / ``qnx_raw_shim`` — creates raw sockets,
       enables ``SO_TIMESTAMPING`` (Linux) or QNX equivalents, performs
       Tx/Rx with hardware timestamps.
   * - **NTP client**
     - ``ntp_client.hpp/cpp`` — sends NTP requests (UDP port 123), applies
       EWMA to offset and jitter for the AbsLoop.

Platform Abstraction
====================

All OS-specific operations are isolated in platform shim modules selected at
compile time via ``-D_QNX_PLAT``:

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Operation
     - Linux (``linux_raw_shim``)
     - QNX (``qnx_raw_shim``)
   * - Raw socket open
     - ``socket(AF_PACKET, SOCK_RAW, ...)``
     - ``qnx_raw_open()``
   * - Hardware Rx timestamp
     - ``SO_TIMESTAMPING`` on ``recvmsg()``
     - QNX NPM timestamping API
   * - Hardware Tx timestamp
     - ``SO_TIMESTAMPING`` + ``CMSG``
     - QNX NPM transmit confirm
   * - PHC step adjustment
     - ``clock_settime(phc_clockid)``
     - ``qnx_phc_adjtime_step()``
   * - PHC frequency trim
     - ``clock_adjtime(ADJ_FREQ)``
     - ``qnx_phc_adjfreq_ppb()``
   * - VLAN support
     - 802.1Q tag insert/strip
     - 802.1Q tag insert/strip

The engine code in ``time_sync_engine.cpp`` never calls platform APIs
directly; it uses only the abstract shim interface, making it fully portable.
