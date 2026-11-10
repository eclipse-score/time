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

Time Daemon Component Requirements
##################################

.. document:: Time Daemon Requirements
   :id: doc__time_daemon_requirements
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, time_daemon

Functional Requirements
-----------------------

Initialization and Lifecycle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Component Initialization
   :id: comp_req__time_daemon__startup
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   On startup the time_daemon component shall apply its configuration and become ready to receive time synchronization data from the time_slave component and to provide time data to the time component (:need:`comp__time`).

.. comp_req:: Startup Timeout
   :id: comp_req__time_daemon__init_timeout
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   If the time_daemon component does not complete its startup within 20 seconds, it shall end its process with a non-zero exit code.

.. comp_req:: Component Shutdown
   :id: comp_req__time_daemon__shutdown
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   On receiving a stop signal (e.g. SIGINT or SIGTERM) the time_daemon component shall stop providing time data and release IPC resources.

.. comp_req:: Start Order Independence
   :id: comp_req__time_daemon__start_order_indep
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall not require the time_slave component to be started before it. If the time_slave component is not yet available on startup, the time_daemon component shall continue its own startup and retry establishing the time synchronization channel with the time_slave component until it succeeds or the startup timeout expires.

Data Reception and Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Time Synchronization Data Reception
   :id: comp_req__time_daemon__time_sync_reception
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall receive the time synchronization data provided by the time_slave component.

Verification Pipeline
^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Synchronization Status Validation
   :id: comp_req__time_daemon__sync_validation
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall validate synchronization status based on received time synchronization data.

.. comp_req:: Synchronization State Stabilization
   :id: comp_req__time_daemon__sync_debounce
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall not report time jumps during the configured time span (default: 5 seconds) after initial synchronization to avoid spuriously detected time jumps during startup.

.. comp_req:: Time Jump Detection
   :id: comp_req__time_daemon__time_jump_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall detect time jumps when the master clock timestamp of a gPTP time sync data update differs from the expected value by more than the configured time span (default: 500 microseconds). Both forward and backward time jumps shall be detected.

.. comp_req:: Timeout Detection
   :id: comp_req__time_daemon__timeout_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall detect timeout condition when no new time synchronization data is received within the configured time span (default: 3.3 seconds).

Data Provision
^^^^^^^^^^^^^^

.. comp_req:: Time Data Provision
   :id: comp_req__time_daemon__time_data_publishing
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall provide the verified time data to the time component (:need:`comp__time`).

.. comp_req:: Provided Time Data Content
   :id: comp_req__time_daemon__published_data_content
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in the time data provided to the time component (:need:`comp__time`, see :need:`comp_req__time_daemon__time_data_publishing`): the timestamp of the master clock, the corresponding timestamp of the local reference clock, the rate deviation between those two clocks, synchronization status, time jump status, and timeout status.

.. comp_req:: Provided Raw gPTP Data
   :id: comp_req__time_daemon__published_raw_data
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in the time data provided to the time component (:need:`comp__time`, see :need:`comp_req__time_daemon__time_data_publishing`) condensed raw data of the last received Sync/FollowUp pair and the last finished Pdelay measurement.

.. comp_req:: Time Point Qualifier Production
   :id: comp_req__time_daemon__time_point_qualifier
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_pt_qual[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall provide the synchronization status, time jump status, and timeout status in the time data provided to the time component (:need:`comp__time`), such that the time component can derive the time point qualifier (quality indicator) from them.

.. comp_req:: Client Read Access
   :id: comp_req__time_daemon__client_read_access
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall provide functionality for client applications to read the time data.

.. comp_req:: Non-Blocking Read Access
   :id: comp_req__time_daemon__non_blocking
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall provide time data such that reading it does not block the reading thread of a client application indefinitely.

.. comp_req:: Multi-Client Support
   :id: comp_req__time_daemon__multi_client
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall allow parallel reads of the provided time data from multiple client applications on different CPU cores.

Error Handling and Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Error Reporting
   :id: comp_req__time_daemon__error_reporting
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync_log[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall log messages via the score::mw::log interface when initialization fails (error level), IPC access fails (error level), verification stage failures occur (warning level), or time synchronization data reception fails (error level).

.. comp_req:: Error Reaction
   :id: comp_req__time_daemon__error_reaction
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1], feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall continue providing time data when a time jump, timeout, or loss of synchronization is detected, with the corresponding status set in the provided time data.

.. comp_req:: Time Jump Recovery
   :id: comp_req__time_daemon__time_jump_recovery
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall clear time jump condition after receiving 2 consecutive valid gPTP frames without time jump.

Platform Abstraction
^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Platform Support
   :id: comp_req__time_daemon__platform_support
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall support Linux and QNX 8.0 operating systems.

.. needextend:: "c.this_doc()"
   :+tags: time_daemon

.. toctree::
   :maxdepth: 1

   chklst_req_inspection
