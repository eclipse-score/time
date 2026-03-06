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

.. _inc_time_dd_ipc:

###
ipc
###

Overview
========

The ``common/ipc`` sub-library (``src/common/``) provides the shared memory
types and utilities used by both ``tsyncd`` and ``libscore_time``. It is
header-only except for ``ShmRegion`` (one ``.cpp`` translation unit).

SharedState
===========

Defined in
``src/common/include/score_time/ipc/shared_state.hpp``.

Header Constants
----------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Constant
     - Value / Purpose
   * - ``kMagic``
     - ``0x53434F52`` (ASCII 'SCOR') — identifies a valid region.
   * - ``kVersion``
     - ``4`` — incremented on every ABI-breaking layout change.
   * - ``kVehicleLogCapacity``
     - ``256`` — number of entries in the vehicle event log.
   * - ``kAbsoluteLogCapacity``
     - ``256`` — number of entries in the absolute event log.

Seqlock Functions
-----------------

Four free functions operate on ``SharedState``:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Function
     - Description
   * - ``WriteVehicle(state, base_ns, base_mono, acc, tpq, offset, delay)``
     - Seqlock write of the vehicle time block.
   * - ``ReadVehicle(state, base_ns, base_mono, acc, tpq) → bool``
     - Seqlock read; returns ``false`` after 1 000 failed retries.
   * - ``WriteAbsolute(state, utc_ns, mono_ns, acc, sec, inaccuracy, ...)``
     - Seqlock write of the absolute time block.
   * - ``ReadAbsolute(state, ...) → bool``
     - Seqlock read of the absolute time block.

Seqlock Correctness
-------------------

The seqlock relies on two guarantees:

1. **Memory ordering** — the writer uses ``memory_order_acq_rel`` on the
   counter and ``memory_order_release`` on payload stores; readers use
   ``memory_order_acquire`` on all loads.
2. **Single writer** — only ``tsyncd`` ever calls ``WriteVehicle`` or
   ``WriteAbsolute``. If multiple writers were present, an additional mutex
   would be required.

Event Log
---------

``LogVehicle()`` and ``LogAbsolute()`` append a ``SyncLogEntry`` to the
respective circular buffer:

1. Atomically increment ``vehicle_log_head`` (or ``abs_log_head``) with
   ``fetch_add`` to claim a slot index.
2. If the slot is already full (wrap-around), increment the drop counter
   and return.
3. Write the entry using the per-entry seqlock protocol.

Each ``SyncLogEntry`` contains:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Field
     - Description
   * - ``seq``
     - 32-bit seqlock counter for this entry.
   * - ``event``
     - ``SyncLogEvent`` enum — identifies the type of event.
   * - ``mono_ns``
     - Monotonic timestamp of the event (``CLOCK_MONOTONIC``).
   * - ``value1``, ``value2``
     - Event-specific payloads (e.g., offset_ns, path_delay_ns).

ShmRegion
=========

Defined in ``src/common/include/score_time/ipc/shm_region.hpp`` and
implemented in ``src/common/shared_memory/shm_region.cpp``.

``ShmRegion`` is a move-only RAII class around ``shm_open`` + ``mmap``:

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Method
     - Description
   * - ``Open(name, size, create_or_open)``
     - Opens or creates the named POSIX shared memory region.
       If ``create_or_open`` is ``true`` and the region does not exist,
       it is created and ``ftruncate`` is called. The region is then
       mapped with ``mmap(PROT_READ|PROT_WRITE, MAP_SHARED)``.
   * - ``Close()``
     - Explicit ``munmap`` + ``close``; also called by destructor.
   * - ``Addr()``
     - Returns the mapped ``void*`` pointer.
   * - ``Size()``, ``Fd()``, ``Valid()``
     - Accessors for the mapped size, file descriptor, and validity.

Utilities
=========

PthreadLockGuard
----------------

``src/common/include/score_time/utils/pthread_lock_guard.hpp`` — an RAII
wrapper for ``pthread_mutex_t``. Used by ``PdelayLoop`` to protect the
peer-delay state machine that is shared between the Rx and Pdelay threads.
Calls ``std::abort()`` on lock/unlock failure (safety-critical assumption:
mutex operations shall not fail in a correctly initialised system).

TimeUtils
---------

``src/common/include/score_time/utils/time_utils.hpp`` — provides
``ClockNs(clockid_t)`` which calls ``clock_gettime`` and converts the
``timespec`` to a single ``int64_t`` nanosecond value, checking for
arithmetic overflow.

StringParser
------------

``src/common/include/score_time/utils/string_parser.hpp`` — exception-free
helpers ``ParseInteger<T>()`` (backed by ``std::from_chars``) and
``ParseDouble()`` (backed by ``strtod``). Used by ``ConfigLoader`` to parse
all numeric configuration values without heap allocation.
