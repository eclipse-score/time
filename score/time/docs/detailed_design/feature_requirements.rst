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

Feature Requirements
====================

.. feat_req:: Unified clock facade across time domains
   :id: feat_req__time__unified_clock_facade
   :reqtype: Interface
   :security: NO
   :safety: QM
   :status: valid
   :version: 1
   :valid_from: v1.0
   :satisfied_by: feat__time

   ``score::time`` shall expose a single, type-safe entry point
   (``Clock<Tag>::GetInstance``) for reading time snapshots across the
   supported clock domains (``VehicleTime``, ``HighResSteadyTime``,
   ``std::chrono::steady_clock``, ``std::chrono::system_clock``), so
   clients select a clock domain at compile time and cannot accidentally
   mix domains at run time.

.. feat_req:: Immutable snapshot with quality metadata
   :id: feat_req__time__snapshot_with_status
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :version: 1
   :valid_from: v1.0
   :satisfied_by: feat__time

   Every ``Clock<Tag>::Now`` call shall return a single immutable
   ``ClockSnapshot`` value that bundles the timepoint with the domain's
   status metadata, so callers can inspect synchronization quality
   without a separate status call.

.. feat_req:: Explicit lifecycle for backends that need it
   :id: feat_req__time__explicit_lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :status: valid
   :version: 1
   :valid_from: v1.0
   :satisfied_by: feat__time

   Clock domains that depend on an external resource (currently
   ``VehicleTime``) shall provide ``Init``, ``IsAvailable`` and
   ``WaitUntilAvailable`` operations, and shall keep those operations
   unavailable — at compile time — on clock domains that are always
   ready.
