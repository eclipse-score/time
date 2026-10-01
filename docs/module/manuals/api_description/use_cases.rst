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

Use Cases
=========

This section describes behavioral patterns for each clock type through sequence diagrams
and focused code snippets. These diagrams show **how** the clock APIs work internally
during typical operations.

For complete, runnable code examples, see :doc:`../examples/index`.

.. _use_cases_basic_clocks:

HighResSteadyTime
-----------------

HT1 — Time Polling
~~~~~~~~~~~~~~~~~~

Measure a short code-path latency or compute a tight deadline where call overhead matters.
``HighResSteadyClock`` avoids a kernel call on QNX by reading the hardware cycle counter
directly — the same ``Now()`` snapshot pattern used for all clock domains, with no status
check required.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/high_res_steady_time/ht1_polling.puml
   :alt: HT1 — HighResSteadyTime time polling

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
   #include <chrono>

   void MyValidator::CheckDeadline()
   {
       auto hirs = score::time::HighResSteadyClock::GetInstance();
       const auto deadline = hirs.Now().TimePoint() + std::chrono::seconds{3};

       // ... do work ...

       if (hirs.Now().TimePoint() > deadline) {
           HandleDeadlineExceeded();
       }
   }

The full working implementation of this pattern is in
``examples/time/high_res_steady_time/src/high_res_steady_time_handler.h`` and
``examples/time/high_res_steady_time/src/main.cpp``.

SteadyClock
-----------

ST1 — Time Polling
~~~~~~~~~~~~~~~~~~

Measure elapsed time between two points, or derive a deadline, using a clock that is
guaranteed never to go backward regardless of external time adjustments.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/steady_clock/st1_polling.puml
   :alt: ST1 — SteadyClock time polling

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/steady_time/src/steady_clock.h"

   void MyComponent::MeasureElapsed()
   {
       auto clock = score::time::SteadyClock::GetInstance();
       const auto start = clock.Now().TimePoint();

       // ... do work ...

       const auto elapsed = clock.Now().TimePoint() - start;
   }

The full working implementation of this pattern is in
``examples/time/steady_time/src/steady_time_handler.h`` and
``examples/time/steady_time/src/main.cpp``.

SystemClock
-----------

SC1 — Time Polling
~~~~~~~~~~~~~~~~~~

Record a wall-clock timestamp for logging or audit trails where the absolute calendar
time matters.  Do not use ``SystemClock`` for elapsed time or timeouts — the timepoint
may jump.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/system_clock/sc1_polling.puml
   :alt: SC1 — SystemClock time polling

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/system_time/src/system_clock.h"
   #include <chrono>

   void MyLogger::LogEvent()
   {
       auto clock = score::time::SystemClock::GetInstance();
       const auto wall_time = clock.Now().TimePoint();
       const auto t = std::chrono::system_clock::to_time_t(wall_time);
       LOG_INFO("Event at: {}", std::ctime(&t));
   }

The full working implementation of this pattern is in
``examples/time/system_time/src/system_time_handler.h`` and
``examples/time/system_time/src/main.cpp``.

.. _use_cases_vehicle_time:

VehicleTime
-----------

VT1 — Time Polling with Status Check
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Obtain a snapshot and inspect the synchronization quality before using the time
value. ``Now()`` returns a single immutable ``ClockSnapshot`` — the timepoint and its
``VehicleTimeStatus`` are always fetched together, with no separate status call needed.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt1_polling.puml
   :alt: VT1 — Time polling with status check

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"

   void MyComponent::CheckTime()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       auto snapshot = clock.Now();

       if (snapshot.Status().IsReliable()) {
           auto tp = snapshot.TimePoint();
           // use tp ...
       } else if (snapshot.Status().IsFlagActive(
                      score::time::VehicleTime::StatusFlag::kTimeOut)) {
           HandleTimeout();
       }
   }

.. note::

   ``Init()`` must be called once during application startup before ``Now()`` is expected
   to return synchronized data (see VT2).  Without it, ``Now()`` returns a snapshot with
   no flags set (``IsConsistent()`` returns ``false``).

**Status flags:**

+---------------------------+------------------------------------------------------------+
| Flag                      | Meaning                                                    |
+===========================+============================================================+
| ``kSynchronized``         | Synchronized at least once to the PTP Grand Master         |
+---------------------------+------------------------------------------------------------+
| ``kTimeOut``              | No sync message received within the configured time window |
+---------------------------+------------------------------------------------------------+
| ``kTimeLeapFuture``       | A large forward adjustment was applied                     |
+---------------------------+------------------------------------------------------------+
| ``kTimeLeapPast``         | A large backward adjustment was applied                    |
+---------------------------+------------------------------------------------------------+

``VehicleTimeStatus::IsReliable()`` returns ``true`` only when ``kSynchronized`` is set
**and** none of ``{kTimeOut, kTimeLeapFuture, kTimeLeapPast}`` is set.
``VehicleTimeStatus::HasBeenSynchronized()`` returns ``true`` whenever ``kSynchronized``
has been set at least once during this lifecycle, regardless of current fault flags.
``VehicleTimeStatus::IsConsistent()`` checks that the flag combination is internally
valid (at least one flag set, and not both leap flags simultaneously).

These three methods belong to ``VehicleTimeStatus`` and encode VehicleTime-domain
semantics.  ``ClockStatus<FlagEnumT>`` itself exposes only generic bit-manipulation
(``IsFlagActive``, ``IsAnyOfFlagsActive``, ``AddFlag``) and the domain-specific
``PrintTo()`` specialization.

The full working implementation of this pattern is in
``examples/time/vehicle_time/src/vehicle_time_handler.h`` (``GetCurrentTime()``) and
``examples/time/vehicle_time/src/main.cpp``.

VT2 — Initialization and Readiness Check
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``VehicleTime`` requires an explicit ``Init()`` call to open the IPC channel to the
time daemon before any time data becomes available.  Until ``Init()`` returns ``true``,
``Now()`` returns a snapshot with no flags set (``IsConsistent()`` returns ``false``) and
``IsAvailable()`` returns ``false``.

After a successful ``Init()``, ``IsAvailable()`` returns ``true`` immediately.  The
non-blocking ``IsAvailable()`` probe and the blocking ``WaitUntilAvailable()`` are useful
when ``Init()`` is retried on a background thread.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt2_availability.puml
   :alt: VT2 — Initialization and readiness check

.. raw:: html

   </div>

**Simple startup (same thread):**

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"

   bool MyService::Startup()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       if (!clock.Init()) {
           LOG_ERROR("VehicleTime: failed to open IPC channel");
           return false;
       }
       auto snapshot = clock.Now();
       // ...
       return true;
   }

**Blocking wait when Init is retried from a background thread:**

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"
   #include <score/stop_token.hpp>
   #include <chrono>

   void MyService::WaitForClock(const score::cpp::stop_token& stop)
   {
       auto clock = score::time::VehicleClock::GetInstance();
       const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
       if (!clock.WaitUntilAvailable(stop, deadline)) {
           LOG_ERROR("VehicleTime did not become available within 30 s");
           return;
       }
       auto snapshot = clock.Now();
       // ...
   }

.. note::

   ``Init()``, ``IsAvailable()``, and ``WaitUntilAvailable()`` are **only available on
   clock domains that require explicit initialisation** (currently ``VehicleTime``).
   Calling them on ``HighResSteadyTime``, ``SteadyClock``, or ``SystemClock`` is a **compile
   error** — those clocks are always ready.

The full working implementation of this pattern is in
``examples/time/vehicle_time/src/vehicle_time_handler.h`` (``Init()``) and
``examples/time/vehicle_time/src/main.cpp``.

VT3 — Async PTP Protocol Data Subscription
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``VehicleTime`` exposes two PTP protocol data callbacks, intended primarily for
diagnostics and PTP data sanity checks:

- ``TimeSlaveSyncData<VehicleTime>`` — fired on each PTP Sync/Follow_Up message pair;
  carries the offset, rate correction, and raw timestamps computed by the TimeSlave.
- ``PDelayMeasurementData<VehicleTime>`` — fired when a peer-delay measurement cycle
  completes; carries the measured peer delay and associated timestamps.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt3_subscription.puml
   :alt: VT3 — Async PTP protocol data subscription

.. raw:: html

   </div>

Delivery is performed by a dedicated worker thread owned by the ``VehicleTime`` backend.
The TimeDaemon publishes into a shared-memory segment without a notification facility, so
the worker polls that segment at a fixed interval (50 ms).  The thread is started when the
first callback is registered and runs until the backend is destroyed; while no callback is
registered it sleeps until the next registration.  A newly registered callback receives the first
snapshot polled after its registration; afterwards it is invoked only for snapshots whose sync or
pDelay content differs from the previously delivered one.

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"
   #include "score/time/ptp/src/time_slave_sync_data.h"
   #include "score/time/ptp/src/pdelay_measurement_data.h"

   void MyDiagHandler::RegisterCallbacks()
   {
       auto clock = score::time::VehicleClock::GetInstance();

       clock.Subscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>(
           [this](const auto& data) { OnTimeSyncData(data); });

       clock.Subscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>(
           [this](const auto& data) { OnPDelayData(data); });
   }

   void MyDiagHandler::Shutdown()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       clock.Unsubscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>();
       clock.Unsubscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>();
   }

.. warning::

   Callbacks are invoked on the **backend thread** — the callback implementation must be
   thread-safe.

VT4 — Synchronization Status Subscription
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Subscribe to ``VehicleTimeStatus`` changes to react when the clock synchronization state
changes — for example, when the timebase becomes synchronized and is ready to use, when a
timeout occurs, or when a large time leap is applied.  This is the primary mechanism for
application components to know that ``VehicleTime`` is reliable and may be safely read.

Unlike the PTP protocol data callbacks in VT3, ``VehicleTimeStatus`` carries no protocol
internals.  It delivers the same status value already available via ``Now().Status()``,
but pushed proactively on every change rather than polled per call.

The callback fires unconditionally on the first PTP status update received after
registration, and subsequently only when the flag set changes.  Rate deviation is
excluded from the comparison.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt4_status_subscription.puml
   :alt: VT4 — Synchronization status subscription

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"

   void MyService::WatchClockReadiness()
   {
       auto clock = score::time::VehicleClock::GetInstance();

       clock.Subscribe<score::time::VehicleTimeStatus>(
           [this](const score::time::VehicleTimeStatus& status) {
               if (status.IsReliable()) {
                   OnClockReady();
               } else if (status.HasBeenSynchronized()) {
                   OnClockDegraded();
               } else {
                   OnClockUnavailable();
               }
           });
   }

   void MyService::Shutdown()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       clock.Unsubscribe<score::time::VehicleTimeStatus>();
   }

.. warning::

   Callbacks are invoked on the **backend thread** — the callback implementation must be
   thread-safe.

The full working implementation of this pattern is in
``examples/time/vehicle_time/src/vehicle_time_handler.h``
(``RegisterStatusCallback()`` / ``UnregisterStatusCallback()``).

VT5 — Status Flag Inspection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When mapping ``VehicleTime`` status to diagnostic outputs such as DTC bitmasks, use
``IsFlagActive(flag)`` with the ``VehicleTime::StatusFlag`` enum to access individual
bits.  For the higher-level reliability predicates (``IsReliable()``,
``HasBeenSynchronized()``), see the status flag table and method descriptions in VT1.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt5_diagnostics.puml
   :alt: VT5 — Status flag inspection

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"
   #include <map>

   using SvtFlag = score::time::VehicleTime::StatusFlag;

   static const std::map<SvtFlag, uint8_t> kDiagBitMap = {
       {SvtFlag::kSynchronized,   0x01U},
       {SvtFlag::kTimeOut,        0x02U},
       {SvtFlag::kTimeLeapFuture, 0x04U},
       {SvtFlag::kTimeLeapPast,   0x08U},
       {SvtFlag::kUnknown,        0x80U},
   };

   uint8_t BuildDiagByte(const score::time::VehicleTimeStatus& status)
   {
       uint8_t result{0U};
       for (const auto& entry : kDiagBitMap) {
           if (status.IsFlagActive(entry.first)) {
               result |= entry.second;
           }
       }
       return result;
   }
