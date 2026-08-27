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

Component Time Requirements
###########################

.. document:: Time Requirements
   :id: doc__time_requirements
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, time

Functional Requirements
-----------------------

.. comp_req:: Unified clock facade across time domains
   :id: comp_req__time__unified_clock_facade
   :reqtype: Interface
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__high_prec_clock_api, feat_req__time__monotonic_clock_api, feat_req__time__abs_base_api, feat_req__time__vehicle_time_time_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``score::time`` shall expose a single, type-safe entry point
   (``Clock<Tag>::GetInstance``) for reading time snapshots across the
   supported clock domains (``VehicleTime``, ``HighResSteadyTime``,
   ``std::chrono::steady_clock``, ``std::chrono::system_clock``), so
   clients select a clock domain at compile time and cannot accidentally
   mix domains at run time.

.. comp_req:: Immutable snapshot with quality metadata
   :id: comp_req__time__snapshot_with_status
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_time_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   Every ``Clock<Tag>::Now`` call shall return a single immutable
   ``ClockSnapshot`` value that bundles the timepoint with the domain's
   status metadata, so callers can inspect synchronization quality
   without a separate status call.

.. comp_req:: Explicit lifecycle for backends that need it
   :id: comp_req__time__explicit_lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_ctrl_flow
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   Clock domains that depend on an external resource (currently
   ``VehicleTime``) shall provide ``Init``, ``IsAvailable`` and
   ``WaitUntilAvailable`` operations, and shall keep those operations
   unavailable — at compile time — on clock domains that are always
   ready.

.. comp_req:: VehicleClock returns snapshot with status
   :id: comp_req__vehicle_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_time_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``VehicleClock::Now`` shall return a ``ClockSnapshot`` whose timepoint
   and ``VehicleTimeStatus`` originate from the same backend read, so
   downstream callers observe consistent time and status values.

.. comp_req:: VehicleClock lifecycle operations
   :id: comp_req__vehicle_time__lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_ctrl_flow
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``VehicleClock`` shall provide ``Init``, ``IsAvailable`` and
   ``WaitUntilAvailable`` operations that delegate to the backend and
   report backend init failure and availability-wait timeouts to the
   caller without blocking indefinitely.

.. comp_req:: HighResSteadyClock always-ready snapshot
   :id: comp_req__high_res_steady_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__high_prec_clock_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``HighResSteadyClock::Now`` shall return a monotonic
   ``ClockSnapshot`` without requiring prior initialization, and shall
   not expose ``Init`` / ``IsAvailable`` / ``WaitUntilAvailable`` on the
   facade (using them is a compile error).

.. comp_req:: SteadyClock always-ready snapshot
   :id: comp_req__steady_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__monotonic_clock_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``SteadyClock::Now`` shall return a snapshot backed by
   ``std::chrono::steady_clock`` without requiring initialization.

.. comp_req:: SystemClock always-ready snapshot
   :id: comp_req__system_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__abs_base_api
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``SystemClock::Now`` shall return a snapshot backed by
   ``std::chrono::system_clock`` without requiring initialization.

.. needextend:: is_external == False and "time" in id
   :+tags: time
