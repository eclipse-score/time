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

Component VehicleTime Requirements
##################################

.. document:: VehicleTime Requirements
   :id: doc__vehicle_time_requirements
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, vehicle_time

Functional Requirements
-----------------------

.. comp_req:: VehicleClock returns snapshot with status
   :id: comp_req__vehicle_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__snapshot_with_status
   :status: valid
   :version: 1
   :satisfied_by: comp__vehicle_time

   ``VehicleClock::Now`` shall return a ``ClockSnapshot`` whose timepoint
   and ``VehicleTimeStatus`` originate from the same backend read, so
   downstream callers observe consistent time and status values.

.. comp_req:: VehicleClock lifecycle operations
   :id: comp_req__vehicle_time__lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__explicit_lifecycle
   :status: valid
   :version: 1
   :satisfied_by: comp__vehicle_time

   ``VehicleClock`` shall provide ``Init``, ``IsAvailable`` and
   ``WaitUntilAvailable`` operations that delegate to the backend and
   report backend init failure and availability-wait timeouts to the
   caller without blocking indefinitely.

.. needextend:: is_external == False and "vehicle_time" in id
   :+tags: vehicle_time
