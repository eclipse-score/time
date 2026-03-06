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

.. _inc_time_comp_req_tsyncd:

############################################
tsyncd — Time Synchronization Daemon
############################################

gPTP Synchronization
====================

.. comp_req:: gPTP Sync and Follow_Up processing
   :id: comp_req__tsyncd__gptp_sync_fup
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__vehicle_time_sync

   The ``tsyncd`` daemon shall receive and parse gPTP ``Sync`` and
   ``Follow_Up`` messages via a raw Ethernet socket (EtherType ``0x88F7``),
   match them by sequence ID, and compute the vehicle clock offset as:

   ``offset = hw_rx_timestamp − (fup_precise_origin + corr_sync + corr_fup)``

   where all timestamps are in nanoseconds.

.. comp_req:: Peer delay measurement
   :id: comp_req__tsyncd__pdelay_measurement
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__vehicle_time_sync

   The ``tsyncd`` daemon shall periodically transmit ``Pdelay_Req`` frames
   and process the corresponding ``Pdelay_Resp`` and
   ``Pdelay_Resp_Follow_Up`` frames to compute the one-way propagation delay
   as:

   ``path_delay = ((t2 − t1) + (t4 − t3_corrected)) / 2``

   The computed delay shall be stored in the vehicle time shared memory block
   and used for offset corrections.

.. comp_req:: PHC clock adjustment
   :id: comp_req__tsyncd__phc_adjustment
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__vehicle_time_sync

   The ``tsyncd`` daemon shall adjust the PTP Hardware Clock (PHC) by
   applying the computed offset via the platform-appropriate PHC API
   (``clock_settime`` on Linux, ``qnx_phc_adjtime_step`` on QNX).

.. comp_req:: Shared memory publication of vehicle time
   :id: comp_req__tsyncd__shm_publish
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__lock_free_ipc, feat_req__time__accuracy_qualifiers

   The ``tsyncd`` daemon shall write the current vehicle clock base, monotonic
   reference timestamp, ``AccuracyQualifier``, ``TimePointQualifier``,
   last offset, and path delay into the vehicle time block of the shared
   memory region using the seqlock write protocol after every successful
   Sync/Follow_Up pair and after every accuracy state transition.

Absolute Time
=============

.. comp_req:: Absolute UTC time acquisition via NTP
   :id: comp_req__tsyncd__ntp_absolute
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__absolute_time

   The ``tsyncd`` daemon shall query one or more NTP servers at a
   configurable interval, apply EWMA smoothing to the offset and jitter
   estimates, and publish the resulting UTC base, inaccuracy, and security
   qualifier to the absolute time block of shared memory once the configured
   number of samples has been collected.

.. comp_req:: Optional external absolute time source
   :id: comp_req__tsyncd__abs_external_source
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__absolute_time

   When enabled, the ``tsyncd`` daemon shall accept absolute UTC time updates
   from an external provider (e.g., GNSS receiver) via a Unix domain socket,
   validate the message format and staleness against a configurable timeout,
   and prefer the external source over NTP when available.

Configuration and Startup
=========================

.. comp_req:: Configuration file loading
   :id: comp_req__tsyncd__config_loading
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__vehicle_time_sync

   The ``tsyncd`` daemon shall parse a ``tsyncd.conf`` configuration file at
   startup, populate all ``EngineOptions`` fields, and abort with a
   descriptive error message if any mandatory parameter is missing or has an
   invalid value.

.. comp_req:: Platform raw socket abstraction
   :id: comp_req__tsyncd__platform_raw_socket
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :satisfies: feat_req__time__cross_platform

   The ``tsyncd`` daemon shall invoke all raw socket, hardware timestamp, and
   PHC operations exclusively through the platform shim interface
   (``linux_raw_shim`` or ``qnx_raw_shim``) selected at compile time via
   ``_QNX_PLAT``, so that the engine logic is fully platform-independent.

Diagnostics
===========

.. comp_req:: Synchronization event logging
   :id: comp_req__tsyncd__event_logging
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :satisfies: feat_req__time__sync_log

   The ``tsyncd`` daemon shall append a ``SyncLogEntry`` to the appropriate
   circular log (vehicle or absolute) in shared memory on every sync state
   transition, offset update, and peer delay sample, using the per-entry
   seqlock write protocol to ensure reader consistency.
