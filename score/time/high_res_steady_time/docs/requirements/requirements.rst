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

Component HighResSteadyTime Requirements
########################################

.. document:: HighResSteadyTime Requirements
   :id: doc__high_res_steady_time_requirements
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, high_res_steady_time

Functional Requirements
-----------------------

.. comp_req:: HighResSteadyClock always-ready snapshot
   :id: comp_req__high_res_steady_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__unified_clock_facade
   :status: valid
   :version: 1
   :satisfied_by: comp__high_res_steady_time

   ``HighResSteadyClock::Now`` shall return a monotonic
   ``ClockSnapshot`` without requiring prior initialization, and shall
   not expose ``Init`` / ``IsAvailable`` / ``WaitUntilAvailable`` on the
   facade (using them is a compile error).

.. needextend:: is_external == False and "high_res_steady_time" in id
   :+tags: high_res_steady_time
