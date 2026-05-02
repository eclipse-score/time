score::time — Unified Clock Interface
======================================

.. contents:: Table of Contents
   :depth: 3
   :local:

Overview
--------

``score::time`` provides a **unified, clock-domain-agnostic API** for reading time snapshots,
subscribing to PTP protocol events, and checking clock readiness — all through a single
template wrapper ``Clock<Tag>``.

The design separates three concerns:

1. **What kind of time** — expressed as a *tag struct* (``VehicleTime``, ``HplsTime``,
   ``std::chrono::steady_clock``, ``std::chrono::system_clock``).
2. **How to access it** — always via ``Clock<Tag>::GetInstance()``; never via a factory class.
3. **How to use it in testing** — via ``test_utils::ScopedClockOverride<Tag>`` (scope-bound global
   override when the SUT calls ``GetInstance()`` internally) or
   ``test_utils::ClockTestFactory<Tag>`` (direct constructor injection, no global state).

Clock domains
~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Clock alias
     - Tag
     - Status concept
   * - ``VehicleClock``
     - ``VehicleTime``
     - ``VehicleTimeStatus`` (PTP sync flags + rate deviation)
   * - ``HplsClock``
     - ``HplsTime``
     - ``NoStatus`` (always-ready local steady clock)
   * - ``SteadyClock``
     - ``std::chrono::steady_clock``
     - ``NoStatus``
   * - ``SystemClock``
     - ``std::chrono::system_clock``
     - ``NoStatus``

Relation to automotive time synchronization standards
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``score::time`` addresses the same problem domain as the time-base management modules
found in automotive middleware stacks: reading a time snapshot, inspecting
synchronization quality flags, waiting for clock availability, and subscribing to
PTP protocol events.

The key design upgrade ``score::time`` brings over typical C-style automotive APIs is
replacing the **runtime integer time-base selector** with a **compile-time ``Tag``
template parameter**.  This gives full type-safety and zero runtime dispatch for
time-domain selection: a component that depends on ``Clock<HplsTime>`` simply cannot
accidentally read ``VehicleTime`` at runtime — the compiler enforces the distinction.
All other structural concepts (composite snapshot result, quality status flags, layered
backend hiding) follow the same principles as established automotive time
synchronization practice, expressed in modern C++.

Architecture
------------

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/architecture_layers.puml
   :alt: Architecture layer diagram

.. raw:: html

   </div>

The library has three layers:

- **Public headers** under ``score/time/<domain>/`` — tag structs and callback types that
  clients include directly.
- **Framework layer** under ``score/time/clock/`` — the ``Clock<Tag>`` wrapper, traits,
  subscription hooks, and the test utilities (``clock_test_utils`` Bazel target).  This
  layer has no backend dependency.
  The ``clock_test_utils`` target (``scoped_clock_override.h``, ``clock_test_factory.h``)
  is ``testonly`` and must not appear in production deps.
- **Internal** under ``score/time/<domain>/details/`` — pure-virtual backend interfaces and
  production implementations.  *Clients must never include anything from a* ``details/``
  *subfolder.*

Class overview
~~~~~~~~~~~~~~

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/class_overview.puml
   :alt: Class overview
   :width: 100%
   :align: center

.. raw:: html

   </div>

Use Cases
---------

UC1 — Time polling with status check
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most common pattern: obtain a snapshot and inspect the synchronization quality
before using the time value.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/uc_polling.puml
   :alt: UC1 — Time polling sequence

.. raw:: html

   </div>

**Code example:**

.. code-block:: cpp

   #include "score/time/vehicle_time/vehicle_clock.h"

   void MyComponent::CheckTime()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       auto snapshot = clock.Now();

       if (snapshot.Status().IsSynchronized()) {
           // Time is valid and synchronized with the PTP Grand Master.
           auto tp = snapshot.TimePoint();
           // use tp ...
       } else if (snapshot.Status().IsFlagActive(
                      score::time::VehicleTime::StatusFlag::kTimeOut)) {
           // Clock has not received a sync message within the timeout window.
           HandleTimeout();
       }
   }

Status flags for ``VehicleTime``:

+---------------------------+--------------------------------------------------------------+
| Flag                      | Meaning                                                      |
+===========================+==============================================================+
| ``kSynchronized``         | Synchronized at least once to the PTP Grand Master           |
+---------------------------+--------------------------------------------------------------+
| ``kSynchToGateway``       | Currently in sync with the PTP gateway                       |
+---------------------------+--------------------------------------------------------------+
| ``kTimeOut``              | No sync message received within the configured time window   |
+---------------------------+--------------------------------------------------------------+
| ``kTimeLeapFuture``       | A large forward adjustment was applied                       |
+---------------------------+--------------------------------------------------------------+
| ``kTimeLeapPast``         | A large backward adjustment was applied                      |
+---------------------------+--------------------------------------------------------------+
| ``kUnknown``              | Status cannot be determined                                  |
+---------------------------+--------------------------------------------------------------+

``IsSynchronized()`` returns ``true`` only when ``kSynchronized`` is set **and** none of
``{kTimeOut, kTimeLeapFuture, kTimeLeapPast}`` is set.

UC2 — Availability waiting at startup
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PTP-backed clocks (``VehicleTime``) require an async connection to the PTP stack.  Use
``IsAvailable()`` for a non-blocking probe, or ``WaitUntilAvailable()`` when it is
acceptable to block the calling thread.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/uc_availability.puml
   :alt: UC2 — Availability waiting sequence

.. raw:: html

   </div>

**Code example:**

.. code-block:: cpp

   #include "score/time/vehicle_time/vehicle_clock.h"
   #include <score/stop_token.hpp>
   #include <chrono>

   void MyService::Init(const score::cpp::stop_token& stop)
   {
       auto clock = score::time::VehicleClock::GetInstance();

       // Non-blocking probe:
       if (!clock.IsAvailable()) {
           LOG_WARN("VehicleTime not yet available, waiting ...");
       }

       // Blocking wait — returns true if ready, false on timeout or stop request:
       const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{30};
       if (!clock.WaitUntilAvailable(stop, deadline)) {
           LOG_ERROR("VehicleTime did not become available within 30 s");
           return;
       }

       auto snapshot = clock.Now();
       // ...
   }

.. note::

   ``IsAvailable()`` and ``WaitUntilAvailable()`` are **only available on clock domains that
   require an async transport** (currently ``VehicleTime``).  Calling them on ``HplsTime``,
   ``SteadyClock``, or ``SystemClock`` is a **compile error** — those clocks are always ready.

UC3 — Async PTP event subscription
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``VehicleTime`` delivers two kinds of PTP protocol events via callbacks:

- ``TimeSlaveSyncData<VehicleTime>`` — fired on each PTP sync message.
- ``PDelayMeasurementData<VehicleTime>`` — fired when a peer-delay measurement completes.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/uc_subscription.puml
   :alt: UC3 — Subscription sequence

.. raw:: html

   </div>

**Code example:**

.. code-block:: cpp

   #include "score/time/vehicle_time/vehicle_clock.h"
   #include "score/time/ptp/time_slave_sync_data.h"
   #include "score/time/ptp/pdelay_measurement_data.h"

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

UC4 — Status flag mapping (diagnostics)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When mapping ``VehicleTime`` status flags to diagnostic bits (e.g. DTC bitmasks), access
the flag type via the tag struct directly:

.. code-block:: cpp

   #include "score/time/vehicle_time/vehicle_clock.h"
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

UC5 — HPLSC as reference steady clock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``HplsClock`` (High-Precision Local Steady Clock) is a monotonic clock that provides
nanosecond-resolution time without any PTP dependency.  It has **no status** — it is always
ready.

.. code-block:: cpp

   #include "score/time/hpls_time/hpls_clock.h"
   #include <chrono>

   void MyValidator::CheckDeadline()
   {
       auto hpls = score::time::HplsClock::GetInstance();
       auto deadline = hpls.Now().TimePoint() + std::chrono::seconds{3};

       // ... do work ...

       if (hpls.Now().TimePoint() > deadline) {
           HandleDeadlineExceeded();
       }
   }

Bazel dependencies
------------------

Choose the target that matches your use case:

.. list-table::
   :header-rows: 1
   :widths: 55 45

   * - Target
     - When to use
   * - ``//score/time/vehicle_time:vehicle_time``
     - Production binary — includes real PTP backend
   * - ``//score/time/vehicle_time:vehicle_time_mock``
     - Unit test — ``VehicleClockMock`` + scope-bound override or constructor injection
   * - ``//score/time/vehicle_time:interface``
     - Header-only, no backend — interface/type usage only
   * - ``//score/time/hpls_time:hpls_time``
     - Production binary — HPLS steady clock
   * - ``//score/time/hpls_time:hpls_time_mock``
     - Unit test — ``HplsClockMock`` + scope-bound override or constructor injection
   * - ``//score/time/hpls_time:interface``
     - Header-only
   * - ``//score/time/steady_time:steady_time``
     - ``std::chrono::steady_clock`` wrapper
   * - ``//score/time/system_time:system_time``
     - ``std::chrono::system_clock`` wrapper
   * - ``//score/time/ptp:ptp_types``
     - PTP notification data types (for callbacks)

Design decisions
----------------

No factory classes
~~~~~~~~~~~~~~~~~~

The old ``SynchronizedVehicleTime`` API required clients to instantiate factory objects
directly.  ``score::time`` removes all factory nesting: ``Clock<Tag>::GetInstance()`` is
the sole entry point, and the production backend is chosen at **link time** by the Bazel
alias target.

Opacity of ``details/``
~~~~~~~~~~~~~~~~~~~~~~~

Virtual dispatch exists solely to enable GMock test doubles.  The vtable is hidden inside
``details/`` — public headers never declare a virtual function.  ``Clock<Tag>`` is a plain
value type.  The ``*_mock.h`` headers are the only public headers permitted to include
``details/`` internals.

``ClockSnapshot`` replaces nested ``TimeStatus``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The old ``SynchronizedVehicleTime::TimeStatus`` had public mutable data members, raw-integer
constructors, and non-const accessors.  ``ClockSnapshot<TimepointT, StatusT>`` is a simple
immutable two-field struct:

.. code-block:: cpp

   auto snap = VehicleClock::GetInstance().Now();
   snap.TimePoint();   // std::chrono::time_point<VehicleTime, nanoseconds>
   snap.Status();      // VehicleTimeStatus — const reference

Generic code works for all clock domains:

.. code-block:: cpp

   template <typename Tag>
   auto Age(score::time::Clock<Tag>& clk,
            typename score::time::Clock<Tag>::time_point ref)
   {
       return clk.Now().TimePoint() - ref;
   }

``Subscribe<E>`` collapses four named callback methods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The old API required four separate named methods (``Set*``, ``Unset*`` for each event type).
The ``Clock<Tag>`` wrapper exposes a single pair ``Subscribe<E>()`` / ``Unsubscribe<E>()``.
The ``SubscriptionHook<Tag, EventType>`` specialisation bridges to the named virtual methods
on the backend interface — which must remain non-template (C++ forbids virtual templates).

Extending with a new clock domain
----------------------------------

Adding a new time domain (e.g. ``SdatTime``) requires only new files — no existing file
is modified:

1. Create ``score/time/sdat_time/sdat_time.h`` — tag struct with ``Duration`` and ``Timepoint``.
2. Create ``score/time/sdat_time/details/sdat_time_iface.h`` — pure-virtual backend interface.
3. Create ``score/time/sdat_time/details/sdat_prod_impl.cpp`` — production backend.
4. Add ``ClockTraits<SdatTime>`` specialisation in ``score/time/sdat_time/sdat_clock.h``.
5. Create ``score/time/sdat_time/sdat_clock_mock.h`` — GMock test double.
6. Add ``sdat_time``, ``sdat_time_mock``, ``interface`` aliases in ``score/time/sdat_time/BUILD``.
