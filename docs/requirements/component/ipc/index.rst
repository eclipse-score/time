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

.. _inc_time_comp_req_ipc:

#################################
ipc — Shared Memory IPC Layer
#################################

SharedState Structure
=====================

.. comp_req:: SharedState ABI versioning
   :id: comp_req__ipc__shared_state
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Interface
   :satisfies: feat_req__time__lock_free_ipc

   The ``SharedState`` structure shall begin with a ``magic`` field
   (``0x53434F52``) and a ``version`` field (currently ``4``) stored as
   ``std::atomic<uint32_t>``, so that readers can detect ABI incompatibility
   and refuse to map a region produced by an incompatible writer version.

Seqlock Protocol
================

.. comp_req:: Seqlock write protocol
   :id: comp_req__ipc__seqlock_write
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__lock_free_ipc

   The ``WriteVehicle()`` and ``WriteAbsolute()`` functions shall implement
   the seqlock write protocol: increment the sequence counter to an odd value
   before modifying any payload field (using ``memory_order_acq_rel``), store
   all payload fields with ``memory_order_release``, then increment the
   counter to an even value with ``memory_order_acq_rel``.

.. comp_req:: Seqlock read protocol with bounded retry
   :id: comp_req__ipc__seqlock_read
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__lock_free_ipc

   The ``ReadVehicle()`` and ``ReadAbsolute()`` functions shall implement the
   seqlock read protocol: load the sequence counter with
   ``memory_order_acquire``, skip the read if the counter is odd (write in
   progress), load all payload fields, then verify that the counter is
   unchanged.  The loop shall retry at most 1000 times and return ``false``
   if the limit is exceeded.

.. comp_req:: Per-entry seqlock for event log
   :id: comp_req__ipc__event_log
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__sync_log

   Each ``SyncLogEntry`` in the circular event log shall carry its own
   32-bit seqlock counter to prevent torn reads of the 64-bit payload fields
   on weakly-ordered memory architectures.  The circular buffer index shall
   be maintained by a separate ``std::atomic<uint64_t>`` head counter, and
   entries that overflow the capacity shall be silently dropped with the drop
   count recorded atomically.

Shared Memory Management
========================

.. comp_req:: POSIX shared memory RAII wrapper
   :id: comp_req__ipc__shm_region
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__lock_free_ipc

   The ``ShmRegion`` class shall provide an RAII wrapper around
   ``shm_open`` / ``mmap`` that:

   - Supports both create-or-open and open-only semantics via a
     ``create_or_open`` flag.
   - Validates the mapped size and alignment before returning a valid
     handle.
   - Automatically unmaps and closes the file descriptor on destruction.
   - Exposes ``Addr()``, ``Size()``, ``Fd()``, and ``Valid()`` accessors.
   - Is move-only (copy constructor and copy assignment are deleted).
