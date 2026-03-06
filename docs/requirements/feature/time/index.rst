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

.. _inc_time_feature_requirements_time:

####################
Feature Requirements
####################

Vehicle Time Synchronization
=============================

.. feat_req:: Vehicle time synchronization via gPTP
   :id: feat_req__time__vehicle_time_sync
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: stkh_req__time__vehicle_time_sync

   The **score::time** feature shall synchronize the local ECU clock with an
   external Time Master using the gPTP protocol (IEEE 802.1AS) by processing
   Sync and Follow_Up messages received over raw Ethernet (EtherType
   ``0x88F7``) and applying the derived offset to the PTP Hardware Clock
   (PHC).

.. feat_req:: Lock-free multi-client IPC
   :id: feat_req__time__lock_free_ipc
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :satisfies: stkh_req__time__multi_client_access

   The **score::time** feature shall publish synchronized time to a POSIX
   shared memory region using a seqlock protocol that allows an unbounded
   number of concurrent readers without blocking the writer and with a
   bounded retry count (≤ 1000 iterations per read).

.. feat_req:: Absolute UTC time via NTP or external source
   :id: feat_req__time__absolute_time
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: stkh_req__time__absolute_time

   The **score::time** feature shall acquire an absolute UTC time reference
   by querying NTP servers and/or consuming an external time source delivered
   via a Unix domain socket, and shall publish the result together with an
   estimated inaccuracy in nanoseconds and a source security qualifier.

.. feat_req:: Synchronization accuracy qualifiers
   :id: feat_req__time__accuracy_qualifiers
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: stkh_req__time__accuracy_reporting

   The **score::time** feature shall attach an ``AccuracyQualifier`` (one of
   ``kNoTime``, ``kNotSynchronized``, ``kSynchronized``, ``kUnstable``,
   ``kTimeJumpDetected``) and a ``TimePointQualifier`` (``kQM`` or
   ``kASIL_B``) to every published vehicle time value, reflecting the current
   synchronization state and integrity level.

.. feat_req:: Cross-platform operation on Linux and QNX
   :id: feat_req__time__cross_platform
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :satisfies: stkh_req__time__platform_support

   The **score::time** feature shall provide identical public API semantics on
   Linux and QNX by isolating all OS-specific behavior (raw socket creation,
   hardware timestamp capture, PHC adjustment) behind compile-time platform
   shims selected via the ``_QNX_PLAT`` preprocessor flag.

.. feat_req:: Synchronization event log
   :id: feat_req__time__sync_log
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: stkh_req__time__diagnostics

   The **score::time** feature shall maintain two circular event logs (vehicle
   and absolute time, each with 256 entries) stored in shared memory, where
   each entry records a ``SyncLogEvent`` type, a monotonic timestamp, and up
   to two payload values, readable by client applications without additional
   IPC calls.
