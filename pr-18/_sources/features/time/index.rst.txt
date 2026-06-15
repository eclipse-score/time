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

1. **What kind of time** — expressed as a *tag struct* (``VehicleTime``, ``HighResSteadyTime``,
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
   * - ``HighResSteadyClock``
     - ``HighResSteadyTime``
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
time-domain selection: a component that depends on ``Clock<HighResSteadyTime>`` simply cannot
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

VehicleTime
~~~~~~~~~~~

VT1 — Time polling with status check
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Obtain a snapshot and inspect the synchronization quality before using the time value.
VehicleTime wraps a PTP-synchronized timebase and always reports a ``VehicleTimeStatus``
alongside the timepoint.

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
   ``kUnknown`` status.

**Status flags:**

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
| ``kUnknown``              | Status cannot be determined (e.g. ``Init()`` not yet called) |
+---------------------------+--------------------------------------------------------------+

``VehicleTimeStatus::IsReliable()`` returns ``true`` only when ``kSynchronized`` is set
**and** none of ``{kTimeOut, kTimeLeapFuture, kTimeLeapPast}`` is set.
``VehicleTimeStatus::HasBeenSynchronized()`` returns ``true`` whenever ``kSynchronized``
has been set at least once during this lifecycle, regardless of current fault flags.
``VehicleTimeStatus::IsConsistent()`` checks that the flag combination is internally
valid (no ``kUnknown``, at least one flag set, not both leap flags simultaneously).

These three methods belong to ``VehicleTimeStatus`` and encode VehicleTime-domain
semantics.  ``ClockStatus<FlagEnumT>`` itself exposes only generic bit-manipulation
(``IsFlagActive``, ``IsAnyOfFlagsActive``, ``AddFlag``) and the domain-specific
``PrintTo()`` specialization.

VT2 — Initialization and readiness check
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``VehicleTime`` requires an explicit ``Init()`` call to open the IPC channel to the
:doc:`TimeDaemon <../time_daemon/index>` before any time data becomes available.  Until ``Init()`` returns ``true``,
``Now()`` returns a snapshot with ``kUnknown`` status and ``IsAvailable()`` returns
``false``.

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

VT3 — Async PTP event subscription
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``VehicleTime`` delivers two kinds of PTP protocol events via callbacks:

- ``TimeSlaveSyncData<VehicleTime>`` — fired on each PTP sync message.
- ``PDelayMeasurementData<VehicleTime>`` — fired when a peer-delay measurement completes.
- ``VehicleTimeStatus`` — fired when the synchronization status flags change (e.g. clock
  becomes synchronized, or a timeout occurs).  The callback fires unconditionally on the
  first event after registration, then only when the flags differ from the last fired value.
  Rate deviation is excluded from the comparison.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt3_subscription.puml
   :alt: VT3 — Async PTP event subscription

.. raw:: html

   </div>

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

       clock.Subscribe<score::time::VehicleTimeStatus>(
           [this](const auto& status) { OnStatusChanged(status); });
   }

   void MyDiagHandler::Shutdown()
   {
       auto clock = score::time::VehicleClock::GetInstance();
       clock.Unsubscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>();
       clock.Unsubscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>();
       clock.Unsubscribe<score::time::VehicleTimeStatus>();
   }

.. warning::

   Callbacks are invoked on the **backend thread** — the callback implementation must be
   thread-safe.

.. note::

   ``VehicleTimeStatus`` subscription is defined in the API and can be registered now.
   The actual delivery is not yet implemented — callbacks will be invoked from a dedicated
   background thread in a future change.

VT4 — Status flag inspection (diagnostics)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When mapping ``VehicleTime`` status flags to diagnostic bits (e.g. DTC bitmasks), access
individual flags via ``VehicleTimeStatus::IsFlagActive()`` using the
``VehicleTime::StatusFlag`` enum.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/vehicle_time/vt4_diagnostics.puml
   :alt: VT4 — Status flag inspection

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

HighResSteadyTime
~~~~~~~~~~~~~~~~~

HT1 — Time polling
^^^^^^^^^^^^^^^^^^^

``HighResSteadyClock`` provides nanosecond-resolution monotonic time with minimal call
overhead.  On QNX the backend reads the hardware cycle counter directly via
``ClockCycles()`` and converts to nanoseconds — no kernel call, no scheduler interaction.
On Linux it calls ``std::chrono::high_resolution_clock::now()``.  It carries ``NoStatus``
— no initialization or quality check is needed.

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

SteadyClock
~~~~~~~~~~~

``SteadyClock`` wraps ``std::chrono::steady_clock`` (POSIX ``CLOCK_MONOTONIC``).  It is
monotonic and never goes backward, making it ideal for measuring elapsed time.  It carries
``NoStatus`` and requires no initialization.

ST1 — Time polling
^^^^^^^^^^^^^^^^^^^

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

SystemClock
~~~~~~~~~~~

``SystemClock`` wraps ``std::chrono::system_clock`` (POSIX ``CLOCK_REALTIME``).  It
represents wall-clock time (UTC-based) and may jump forward or backward on NTP
adjustments or manual time changes.  It carries ``NoStatus`` and requires no
initialization.  Use it when you need a calendar timestamp — not for measuring elapsed
time.

SC1 — Time polling
^^^^^^^^^^^^^^^^^^^

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

Testing (all domains)
~~~~~~~~~~~~~~~~~~~~~

Both test utilities work with any clock domain.  Choose based on how the SUT obtains the
clock:

+------------------------------+-------------------------------------------------------+
| Utility                      | When to use                                           |
+==============================+=======================================================+
| ``ScopedClockOverride<Tag>`` | SUT calls ``Clock<Tag>::GetInstance()`` internally.   |
|                              | Scope-bound RAII guard — automatically restored on    |
|                              | destruction.                                          |
+------------------------------+-------------------------------------------------------+
| ``ClockTestFactory<Tag>``    | SUT accepts ``Clock<Tag>`` as a constructor argument. |
|                              | No global state is touched — safe for parallel tests. |
+------------------------------+-------------------------------------------------------+

T1 — ScopedClockOverride (scope-bound override)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/testing/t1_scoped_override.puml
   :alt: T1 — ScopedClockOverride

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"
   #include "score/time/vehicle_time/src/vehicle_clock_backend_mock.h"

   TEST(MyServiceTest, ReportsReliableTime)
   {
       auto mock = std::make_shared<score::time::VehicleClockBackendMock>();
       EXPECT_CALL(*mock, Now()).WillOnce(Return(/* snapshot */));

       const score::time::test_utils::ScopedClockOverride<score::time::VehicleTime> guard{mock};

       MyService svc;
       svc.DoSomething();  // calls VehicleClock::GetInstance() internally
   }

T2 — ClockTestFactory (constructor injection)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/testing/t2_test_factory.puml
   :alt: T2 — ClockTestFactory

.. raw:: html

   </div>

.. code-block:: cpp

   #include "score/time/vehicle_time/src/vehicle_clock.h"
   #include "score/time/vehicle_time/src/vehicle_clock_backend_mock.h"

   TEST(MyServiceTest, ReportsReliableTime)
   {
       auto mock = std::make_shared<score::time::VehicleClockBackendMock>();
       EXPECT_CALL(*mock, Now()).WillOnce(Return(/* snapshot */));

       const auto clock =
           score::time::test_utils::ClockTestFactory<score::time::VehicleTime>::Make(mock);

       MyService svc{clock};
       svc.DoSomething();
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
     - Unit test — ``VehicleClockBackendMock`` + scope-bound override or constructor injection
   * - ``//score/time/vehicle_time:interface``
     - Header-only, no backend — interface/type usage only
   * - ``//score/time/high_res_steady_time:high_res_steady_time``
     - Production binary — HIRS steady clock
   * - ``//score/time/high_res_steady_time:high_res_steady_time_mock``
     - Unit test — ``HighResSteadyClockBackendMock`` + scope-bound override or constructor injection
   * - ``//score/time/high_res_steady_time:interface``
     - Header-only
   * - ``//score/time/steady_time:steady_time``
     - ``std::chrono::steady_clock`` wrapper
   * - ``//score/time/system_time:system_time``
     - ``std::chrono::system_clock`` wrapper
   * - ``//score/time/ptp:ptp_types``
     - PTP notification data types (``TimeSlaveSyncData``, ``PDelayMeasurementData``)
   * - ``//score/time/vehicle_time:interface``
     - Required when subscribing to ``VehicleTimeStatus`` — provides the type definition

Design decisions
----------------

Single entry point — no factory classes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Classical time APIs expose a factory or manager object that clients instantiate and
configure before reading time (e.g. ``TimeBaseManager tm; tm.GetCurrentTime(kVehicleBase)``).
``score::time`` removes that level of indirection: ``Clock<Tag>::GetInstance()`` is the
sole entry point, and the production backend is chosen at **link time** by the Bazel
alias target.

Opacity of ``details/``
~~~~~~~~~~~~~~~~~~~~~~~

Virtual dispatch exists solely to enable GMock test doubles.  The vtable is hidden inside
``details/`` — public headers never declare a virtual function.  ``Clock<Tag>`` is a plain
value type.  The ``*_mock.h`` headers are the only public headers permitted to include
``details/`` internals.

``ClockSnapshot`` — immutable composite result
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Classical time APIs return a raw timestamp and require a separate call to retrieve the
synchronization status, or expose a struct with public mutable data members and
raw-integer constructors.  ``ClockSnapshot<TimepointT, StatusT>`` is a simple
immutable two-field struct:

.. code-block:: cpp

   auto snap = VehicleClock::GetInstance().Now();
   snap.TimePoint();   // std::chrono::time_point<VehicleTime, nanoseconds>
   snap.Status();      // VehicleTimeStatus — returned by value

Generic code works for all clock domains:

.. code-block:: cpp

   template <typename Tag>
   auto Age(score::time::Clock<Tag>& clk,
            typename score::time::Clock<Tag>::time_point ref)
   {
       return clk.Now().TimePoint() - ref;
   }

``Subscribe<E>`` — uniform subscription API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Classical event-callback APIs require a separate named setter and unsetter for each event
type (e.g. ``SetSyncDataCallback``, ``UnsetSyncDataCallback``, ``SetPDelayCallback``,
``UnsetPDelayCallback``).  ``Clock<Tag>`` exposes a single pair ``Subscribe<E>()`` /
``Unsubscribe<E>()`` templated on the event type.
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
7. *(If the new domain requires explicit initialisation)* Add a full specialisation of
   ``InitializationHook<SdatTime>`` in ``sdat_clock.h`` supplying
   ``static bool CallInit(Backend&) noexcept``.  This makes ``Clock<SdatTime>::Init()``
   available at compile time without touching any existing files.
8. *(If the new domain requires readiness checking)* Add a full specialisation of
   ``AvailabilityHook<SdatTime>`` in ``sdat_clock.h`` supplying
   ``static bool CallIsAvailable(const Backend&)`` and
   ``static bool CallWaitUntilAvailable(const Backend&, stop_token, time_point)``.
   This makes ``IsAvailable()`` and ``WaitUntilAvailable()`` available at compile time.
