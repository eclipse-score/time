..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

.. _inc_time_comp_req_libscore_time:

###################################
libscore_time — Client Library
###################################

Public API
==========

.. comp_req:: Vehicle time client API
   :id: comp_req__libscore_time__vehicle_time_api
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Interface
   :satisfies: feat_req__time__vehicle_time_sync

   The ``libscore_time`` library shall expose an ``IVehicleTime`` interface
   providing:

   - ``Now()`` — returns a ``TimePoint<VehicleClockTag>`` containing the
     current gPTP-synchronized vehicle time in nanoseconds.
   - ``GetAccuracyQualifier()`` — returns the current ``AccuracyQualifier``
     reflecting the synchronization state.
   - ``GetTimePointQualifier()`` — returns the current ``TimePointQualifier``
     reflecting the ASIL integrity level of the time value.

.. comp_req:: Absolute time client API
   :id: comp_req__libscore_time__absolute_time_api
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Interface
   :satisfies: feat_req__time__absolute_time

   The ``libscore_time`` library shall expose an ``IAbsoluteTime`` interface
   providing:

   - ``Now()`` — returns a ``TimePoint<AbsoluteClockTag>`` in nanoseconds
     since the Unix epoch.
   - ``GetAccuracyQualifier()`` — returns the UTC inaccuracy range as an
     ``AbsoluteAccuracyQualifier``.
   - ``GetSecurityQualifier()`` — returns the ``AbsoluteSecurityQualifier``
     indicating source trustworthiness.
   - ``GetEstimatedInaccuracyNs()`` — returns the estimated UTC inaccuracy
     in nanoseconds.

.. comp_req:: Accuracy qualifier API
   :id: comp_req__libscore_time__accuracy_api
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Interface
   :satisfies: feat_req__time__accuracy_qualifiers

   The ``libscore_time`` library shall expose the ``AccuracyQualifier``,
   ``TimePointQualifier``, ``AbsoluteAccuracyQualifier``, and
   ``AbsoluteSecurityQualifier`` enumeration types as part of the public API
   so that callers can inspect and branch on synchronization quality without
   depending on internal implementation details.

.. comp_req:: Diagnostic log read API
   :id: comp_req__libscore_time__sync_log_api
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Interface
   :satisfies: feat_req__time__sync_log

   The ``libscore_time`` library shall provide ``ReadVehicleSyncLog()`` and
   ``ReadAbsoluteSyncLog()`` methods on ``ScoreTime`` that copy up to
   *N* ``SyncLogEntry`` records from the shared memory circular log into a
   caller-supplied buffer and return the number of entries copied.

Factory and Lifecycle
=====================

.. comp_req:: ScoreTime factory method
   :id: comp_req__libscore_time__factory
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__lock_free_ipc

   The ``libscore_time`` library shall provide a
   ``ScoreTime::Create(Options)`` static factory that opens (or creates) the
   POSIX shared memory region named by ``Options::shm_name``, validates the
   ``magic`` and ``version`` fields, and returns a fully initialized
   ``ScoreTime`` object.  The factory shall return a null pointer if the
   shared memory region cannot be opened or fails validation.

.. comp_req:: Lock-free shared memory read
   :id: comp_req__libscore_time__seqlock_read
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :satisfies: feat_req__time__lock_free_ipc

   Every ``IVehicleTime::Now()`` and ``IAbsoluteTime::Now()`` call shall
   read from shared memory exclusively via the seqlock read protocol
   (maximum 1000 retry iterations) without acquiring any OS mutex or
   performing any system call.
