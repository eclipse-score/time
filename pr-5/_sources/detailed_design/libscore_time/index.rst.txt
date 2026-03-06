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

.. _inc_time_dd_libscore_time:

#############
libscore_time
#############

Overview
========

``libscore_time`` is a header-plus-one-TU library that maps the shared memory
region produced by ``tsyncd`` and exposes four typed clock interfaces. All
reads are lock-free; the library never calls ``pthread_mutex_lock`` or any
blocking system call on the hot path.

Public Header
=============

The single public header is ``src/libscore_time/include/score_time.hpp``.
Clients include it as:

.. code-block:: cpp

   #include <score_time.hpp>

ScoreTime — Factory and Facade
================================

``ScoreTime`` is a move-only class. Construction is via the static factory:

.. code-block:: cpp

   static std::unique_ptr<ScoreTime> Create(Options options = {});

The factory:

1. Calls ``ShmRegion::Open(options.shm_name, options.shm_size, false)`` to
   map the region read-only.
2. Validates ``SharedState::magic == kMagic`` and
   ``SharedState::version == kVersion``; returns ``nullptr`` on mismatch.
3. Constructs internal ``VehicleTimeImpl`` and ``AbsoluteTimeImpl`` objects
   that hold a pointer into the mapped region.

Options
-------

.. list-table::
   :header-rows: 1
   :widths: 35 30 35

   * - Field
     - Default
     - Description
   * - ``shm_name``
     - ``/score_time_vehicle_time``
     - POSIX shared memory name to open.
   * - ``shm_size``
     - 16384
     - Expected region size (validated against actual mapping).
   * - ``high_precision_mapping``
     - ``kRealtime``
     - Whether ``IHighPrecisionClock::Now()`` reads ``CLOCK_REALTIME``
       (``kRealtime``) or ``CLOCK_TAI`` (``kTai``).

Clock Interfaces
================

IVehicleTime
------------

.. code-block:: cpp

   class IVehicleTime {
   public:
       virtual TimePoint<VehicleClockTag>  Now() const = 0;
       virtual AccuracyQualifier           GetAccuracyQualifier() const = 0;
       virtual TimePointQualifier          GetTimePointQualifier() const = 0;
   };

``Now()`` implementation inside ``VehicleTimeImpl``:

1. Reads ``vehicle_base_mono_ns`` and ``vehicle_base_ns`` from shared memory
   via the seqlock read protocol (max 1 000 retries).
2. Calls ``ClockNs(CLOCK_MONOTONIC)`` to get the current monotonic time.
3. Returns ``TimePoint{vehicle_base_ns + (mono_now − vehicle_base_mono_ns)}``.

``GetAccuracyQualifier()`` and ``GetTimePointQualifier()`` load their
respective atomics with ``memory_order_acquire``; no seqlock needed because
these are single-word atomic stores.

IAbsoluteTime
-------------

.. code-block:: cpp

   class IAbsoluteTime {
   public:
       virtual TimePoint<AbsoluteClockTag>  Now() const = 0;
       virtual AbsoluteAccuracyQualifier    GetAccuracyQualifier() const = 0;
       virtual AbsoluteSecurityQualifier    GetSecurityQualifier() const = 0;
       virtual int64_t                      GetEstimatedInaccuracyNs() const = 0;
   };

``Now()`` follows the same interpolation pattern as ``IVehicleTime::Now()``
using ``abs_base_utc_ns`` and ``abs_base_mono_ns``.

IHighPrecisionClock / IMonotonicClock
--------------------------------------

These are thin wrappers around ``ClockNs(CLOCK_REALTIME)`` /
``ClockNs(CLOCK_TAI)`` and ``ClockNs(CLOCK_MONOTONIC)`` respectively,
both implemented as direct ``clock_gettime`` calls.

Diagnostic API
==============

.. code-block:: cpp

   std::size_t ReadVehicleSyncLog(SyncLogEntry* out, std::size_t max_count);
   std::size_t ReadAbsoluteSyncLog(SyncLogEntry* out, std::size_t max_count);

These methods copy up to ``max_count`` entries from the circular ring buffer
in shared memory. Each entry is read using its per-entry seqlock (retry
limit 1 000). Returns the number of entries actually copied.

Type System
===========

``TimePoint<Tag>`` is a ``std::chrono``-compatible strong typedef:

.. code-block:: cpp

   template <typename Tag>
   struct TimePoint {
       int64_t count() const;  // nanoseconds since epoch
   };

   using VehicleTimePoint   = TimePoint<VehicleClockTag>;
   using AbsoluteTimePoint  = TimePoint<AbsoluteClockTag>;
   using MonotonicTimePoint = TimePoint<MonotonicClockTag>;

The tag types prevent accidental mixing of vehicle time with monotonic time
at compile time.

Accuracy Qualifiers
-------------------

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - ``AccuracyQualifier``
     - Meaning
   * - ``kNoTime``
     - Shared memory not yet written by tsyncd.
   * - ``kNotSynchronized``
     - tsyncd is running but no gPTP master has been seen.
   * - ``kSynchronized``
     - Successfully locked to gPTP master, offset within thresholds.
   * - ``kUnstable``
     - Offset exceeds ``unstable_offset_threshold_ns``.
   * - ``kTimeJumpDetected``
     - Offset exceeds ``jump_future_threshold_ns`` (large step).

Test Mocks
==========

``src/libscore_time/include/mocks.hpp`` provides ``ManualVehicleTime`` and
``ManualAbsoluteTime`` — hand-written test doubles that implement the
clock interfaces and allow direct injection of time values and accuracy
qualifiers in unit tests without requiring a running ``tsyncd`` daemon.
