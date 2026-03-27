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

.. _libtsclient_design:

############################
libTSClient Design
############################

.. contents:: Table of Contents
   :depth: 3
   :local:

Overview
========

**libTSClient** is the shared memory IPC library that connects the TimeSlave process to the
TimeDaemon process. It provides a lock-free, single-writer/multi-reader communication
channel using a **seqlock protocol** over POSIX shared memory.

The library is intentionally minimal — it consists of three headers and two source files —
to keep the IPC boundary simple, auditable, and suitable for safety-critical deployments.

Architecture
============

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_channel.puml
   :alt: libTSClient IPC Architecture

.. raw:: html

   </div>

Components
==========

GptpIpcChannel
--------------

Defines the shared memory layout as the ``GptpIpcRegion`` structure:

.. list-table::
   :header-rows: 1
   :widths: 20 20 60

   * - Field
     - Type
     - Purpose
   * - ``magic``
     - ``uint32_t``
     - Validation constant (``0x47505440``). Used by the receiver to confirm the shared
       memory segment is valid and initialized.
   * - ``seq``
     - ``std::atomic<uint32_t>``
     - Seqlock counter. Odd values indicate a write in progress; even values indicate
       a consistent state.
   * - ``data``
     - ``PtpTimeInfo``
     - The actual time synchronization payload (PTP status, Sync/FollowUp data,
       peer delay data, local clock reference).

The structure is aligned to 64 bytes (cache line size) to prevent false sharing between
the writer and reader processes.

The default POSIX shared memory name is ``/gptp_ptp_info`` (defined as ``kGptpIpcName``).

GptpIpcPublisher
----------------

The **single-writer** component, used by TimeSlave:

- ``Init(name)`` — Creates the POSIX shared memory segment via ``shm_open(O_CREAT | O_RDWR)``,
  sizes it with ``ftruncate()``, maps it with ``mmap(PROT_READ | PROT_WRITE)``, and writes
  the magic number.

- ``Publish(info)`` — Writes a ``PtpTimeInfo`` using the seqlock protocol:

  1. Increment ``seq`` (becomes odd — signals write in progress)
  2. ``memcpy`` the data
  3. Increment ``seq`` (becomes even — signals write complete)

- ``Destroy()`` — Unmaps and unlinks the shared memory segment.

GptpIpcReceiver
---------------

The **multi-reader** component, used by the TimeDaemon (via ``RealPTPEngine``):

- ``Init(name)`` — Opens the existing shared memory segment via ``shm_open(O_RDONLY)`` and
  maps it with ``mmap(PROT_READ)``. Validates the magic number.

- ``Receive()`` — Reads ``PtpTimeInfo`` using the seqlock protocol with up to 20 retries:

  1. Read ``seq`` (must be even, otherwise retry)
  2. ``memcpy`` the data
  3. Read ``seq`` again (must match step 1, otherwise retry — torn read detected)
  4. Return ``std::optional<PtpTimeInfo>`` (empty if all retries exhausted)

- ``Close()`` — Unmaps the shared memory.

Seqlock protocol
================

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/ipc_sequence.puml
   :alt: Seqlock IPC Protocol Sequence

.. raw:: html

   </div>

The seqlock provides the following properties:

- **Lock-free for readers** — Readers never block the writer. A torn read is detected and
  retried transparently.
- **Single writer** — Only one process (TimeSlave) writes to the shared memory. No
  writer-writer contention.
- **Bounded retry** — The receiver retries up to 20 times. Under normal operation,
  retries are rare because the write is a single ``memcpy`` of a small struct.
- **Cache-line alignment** — The 64-byte alignment of ``GptpIpcRegion`` prevents false
  sharing, which is critical for cross-process shared memory performance.

Data type
=========

The ``PtpTimeInfo`` structure (defined in ``score/TimeDaemon/code/common/data_types/ptp_time_info.h``)
is the payload transferred through the IPC channel. It contains:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Field
     - Content
   * - ``PtpStatus``
     - Synchronization flags (synchronized, timeout, time leap indicators)
   * - ``SyncFupData``
     - Sync and FollowUp message timestamps and correction fields
   * - ``PDelayData``
     - Peer delay measurement results
   * - Local clock value
     - Reference timestamp from ``HighPrecisionLocalSteadyClock``

Build integration
=================

The library is built as a Bazel ``cc_library`` target:

.. code-block:: text

   //score/libTSClient:gptp_ipc

It links against ``-lrt`` for POSIX shared memory (``shm_open``, ``shm_unlink``) and
depends on the ``PtpTimeInfo`` data type from the TimeDaemon common module.

Both TimeSlave and TimeDaemon link against ``libTSClient`` — the publisher side in
TimeSlave, the receiver side in TimeDaemon.
