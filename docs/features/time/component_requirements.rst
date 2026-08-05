Component Requirements
======================

.. comp_req:: VehicleClock returns snapshot with status
   :id: comp_req__time__vehicle_clock_snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :satisfies: feat_req__time__snapshot_with_status

   ``VehicleClock::Now`` shall return a ``ClockSnapshot`` whose timepoint
   and ``VehicleTimeStatus`` originate from the same backend read, so
   downstream callers observe consistent time and status values.

.. comp_req:: VehicleClock lifecycle operations
   :id: comp_req__time__vehicle_clock_lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :satisfies: feat_req__time__explicit_lifecycle

   ``VehicleClock`` shall provide ``Init``, ``IsAvailable`` and
   ``WaitUntilAvailable`` operations that delegate to the backend and
   report backend init failure and availability-wait timeouts to the
   caller without blocking indefinitely.

.. comp_req:: HighResSteadyClock always-ready snapshot
   :id: comp_req__time__hirs_clock_snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :satisfies: feat_req__time__unified_clock_facade

   ``HighResSteadyClock::Now`` shall return a monotonic
   ``ClockSnapshot`` without requiring prior initialization, and shall
   not expose ``Init`` / ``IsAvailable`` / ``WaitUntilAvailable`` on the
   facade (using them is a compile error).

.. comp_req:: SteadyClock always-ready snapshot
   :id: comp_req__time__steady_clock_snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :satisfies: feat_req__time__unified_clock_facade

   ``SteadyClock::Now`` shall return a snapshot backed by
   ``std::chrono::steady_clock`` without requiring initialization.

.. comp_req:: SystemClock always-ready snapshot
   :id: comp_req__time__system_clock_snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :satisfies: feat_req__time__unified_clock_facade

   ``SystemClock::Now`` shall return a snapshot backed by
   ``std::chrono::system_clock`` without requiring initialization.
