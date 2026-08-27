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

.. comp_req:: VehicleClock returns snapshot with status
   :id: comp_req__vehicle_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
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
   :status: valid
   :version: 1
   :satisfied_by: comp__time

   ``SystemClock::Now`` shall return a snapshot backed by
   ``std::chrono::system_clock`` without requiring initialization.

.. needextend:: is_external == False and "time" in id
   :+tags: time
