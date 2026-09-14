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
   :id: comp_req__time_daemon__initialization
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   On startup the time_daemon component shall determine its configuration, initialize the configured time synchronization data receiver, time data verification pipeline (synchronization validation, timeout detection, time jump detection), and IPC publisher. If initialization of any of those units fails, the time_daemon shall retry to initialize the respective unit. Overall initialization shall fail if not completed within 20 seconds and shall end the time_slave process.

.. comp_req:: Component Shutdown
   :id: comp_req__time_daemon__shutdown
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall stop the publishing loop and close plus release any IPC resources (e.g. shared memory mappings) when a stop signal is received.

Data Reception and Validation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Time Synchronization Data Reception
   :id: comp_req__time_daemon__gptp_shm_reception
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
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1],
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in published time data: the timestamp of the master clock, the corresponding timestamp of the local reference clock, the rate deviation between those two clocks, synchronization status, time jump status, and timeout status.
   
   Also it shall include condensed raw data of the last received Sync/FollowUp pair and the last finished Pdelay measurement.

.. comp_req:: Time Point Qualifier Production
   :id: comp_req__time_daemon__time_point_qualifier
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_pt_qual[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall determine the time point qualifier (quality indicator) of the published time data from the outcome of the synchronization, time jump and timeout verification checks.

.. comp_req:: Publish Interval
   :id: comp_req__time_daemon__publish_interval
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall publish time data to client applications at a fixed configurable interval (default: 250 milliseconds).

.. comp_req:: Non-Blocking Access Path
   :id: comp_req__time_daemon__non_blocking
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall use a non-blocking, lock-free shared memory access path for receiving time synchronization data and providing published time data to client applications.

.. comp_req:: Multi-Client Support
   :id: comp_req__time_daemon__multi_client
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall support concurrent non-blocking read access from multiple client applications to the published time data.

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

   The time_daemon component shall log messages via the score::mw::log interface when initialization fails (error level), shared memory access fails (error level), verification stage failures occur (warning level), or time synchronization data reception fails (error level).

.. comp_req:: Time Jump Error Reaction
   :id: comp_req__time_daemon__time_jump_reaction
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall continue publishing time data with time jump status set when time jump is detected.

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

.. comp_req:: Timeout Error Reaction
   :id: comp_req__time_daemon__timeout_reaction
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall continue publishing time data with timeout status set when the timeout detection period elapses without receiving new gPTP data.

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

   The time_daemon component shall support POSIX and QNX 8.0 SDPplatforms for shared memory access and IPC communication.

Assumption of Use Requirements
-------------------------------

.. aou_req:: gPTP Shared Memory Availability
   :id: aou_req__time_daemon__gptp_shm_available
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1

   The user shall ensure gPTP shared memory is initialized and the time_slave component is running before starting time_daemon. Starting time_daemon without an initialized shared memory region will cause shared memory access failures or reading of stale/uninitialized time data during startup.


.. needextend:: "c.this_doc()"
   :+tags: time_daemon

.. toctree::
   :maxdepth: 1

   chklst_req_inspection
