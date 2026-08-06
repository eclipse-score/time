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

Component SystemTime Requirements
#################################

.. document:: SystemTime Requirements
   :id: doc__system_time_requirements
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, system_time

Functional Requirements
-----------------------

.. comp_req:: SystemClock always-ready snapshot
   :id: comp_req__system_time__snapshot
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__unified_clock_facade
   :status: valid
   :version: 1
   :satisfied_by: comp__system_time

   ``SystemClock::Now`` shall return a snapshot backed by
   ``std::chrono::system_clock`` without requiring initialization.

.. needextend:: is_external == False and "system_time" in id
   :+tags: system_time
