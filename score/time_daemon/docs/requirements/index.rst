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

   On startup the time_daemon component shall determine its configuration and initialize the configured time synchronization data receiver, time data verification pipeline (synchronization validation, timeout detection, time jump detection), and IPC publisher.

.. comp_req:: Initialization Retry
   :id: comp_req__time_daemon__init_retry
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   If initialization of the time synchronization data receiver, time data verification pipeline, or IPC publisher fails, the time_daemon component shall retry to initialize the respective unit.

.. comp_req:: Initialization Timeout
   :id: comp_req__time_daemon__init_timeout
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   If initialization is not completed within 20 seconds, the time_daemon component shall fail initialization and end the time_daemon process.

.. comp_req:: Component Shutdown
   :id: comp_req__time_daemon__shutdown
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   On receiving a stop signal (e.g. SIGINT or SIGTERM) the time_daemon component shall stop publishing time data and release IPC resources.

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

   The time_daemon component shall receive time synchronization data from time_slave component using the ts_client component for IPC abstraction.

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

Data Publishing
^^^^^^^^^^^^^^^

.. comp_req:: Time Data Publishing
   :id: comp_req__time_daemon__time_data_publishing
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall publish the verified time synchronization data to the time component linked as a library into client applications.

.. comp_req:: Published Time Data Content
   :id: comp_req__time_daemon__published_data_content
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in published time data: the timestamp of the master clock, the corresponding timestamp of the local reference clock, the rate deviation between those two clocks, synchronization status, time jump status, and timeout status.

.. comp_req:: Published Raw gPTP Data
   :id: comp_req__time_daemon__published_raw_data
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in published time data condensed raw data of the last received Sync/FollowUp pair and the last finished Pdelay measurement.

.. comp_req:: Time Point Qualifier Production
   :id: comp_req__time_daemon__time_point_qualifier
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_pt_qual[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall derive the time point qualifier (quality indicator) of the time data provided to the time component from the synchronization status, time jump status, and timeout status.

.. comp_req:: Client Read Access
   :id: comp_req__time_daemon__client_read_access
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall provide the time component with functionality to read the published time data.

.. comp_req:: Non-Blocking Read Access
   :id: comp_req__time_daemon__non_blocking
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   Reading the published time data shall not block the reading thread of a client application indefinitely.

.. comp_req:: Multi-Client Support
   :id: comp_req__time_daemon__multi_client
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall allow parallel reads of the published time data from multiple client applications on different CPU cores.

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

   The time_daemon component shall continue publishing time data when a time jump, timeout, or loss of synchronization is detected, with the corresponding status set in the published time data.

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

   The time_daemon component shall support Linux and QNX 8.0 SDP platforms.

Assumption of Use Requirements
-------------------------------

.. aou_req:: Time Slave Availability
   :id: aou_req__time_daemon__time_slave_running
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The user shall ensure the time_slave component is running before starting time_daemon, or is started such that it becomes available within the time_daemon initialization timeout. Otherwise time_daemon initialization will fail.


.. needextend:: "c.this_doc()"
   :+tags: time_daemon

.. toctree::
   :maxdepth: 1

   chklst_req_inspection
