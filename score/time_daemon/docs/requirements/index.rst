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

Component Time Daemon Requirements
##################################

.. document:: Time Daemon Requirements
   :id: doc__time_daemon_requirements
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: time_daemon

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

   The time_daemon component shall initialize the gPTP shared memory reader, time data verification (synchronization validation, time jump detection, timeout detection), and IPC publisher during initialization. Initialization shall fail if not completed within 20 seconds.

.. comp_req:: Component Shutdown
   :id: comp_req__time_daemon__shutdown
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall stop the publishing loop, release shared memory mappings, and close IPC resources when deinitialized or when a stop signal is received.

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

   The time_daemon component shall receive time synchronization data from the gPTP shared memory region written by the time_slave component.

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

   The time_daemon component shall not report synchronization state changes during the first 5 seconds after initial synchronization to avoid spurious time jump detection during startup.

.. comp_req:: Time Jump Detection
   :id: comp_req__time_daemon__time_jump_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall detect time jumps when consecutive gPTP frames differ by more than 500 microseconds. Both forward and backward time jumps shall be detected.

.. comp_req:: Timeout Detection
   :id: comp_req__time_daemon__timeout_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall detect timeout condition when no new time synchronization frame is received within 3.3 seconds.

Data Publishing
^^^^^^^^^^^^^^^

.. comp_req:: Time Data Publishing
   :id: comp_req__time_daemon__time_data_publishing
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall publish time data that has passed verification checks to client applications via the VehicleTime IPC interface.

.. comp_req:: Published Time Data Content
   :id: comp_req__time_daemon__published_data_content
   :reqtype: Interface
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_time_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall include in published time data: time value, synchronization status, time jump status, and timeout status.

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

   The time_daemon component shall publish time data to client applications at a fixed interval of maximum 250 milliseconds to provide consistent read latency for VehicleTime clients.

.. comp_req:: Periodic Publishing Fallback
   :id: comp_req__time_daemon__periodic_fallback
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall republish the last valid time data at the publishing interval when no new upstream data has arrived.

.. comp_req:: Multi-Client Support
   :id: comp_req__time_daemon__multi_client
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall support concurrent read access from multiple client applications to the published time data.

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

   The time_daemon component shall continue publishing time data with timeout status set when the timeout detection period elapses without new gPTP frames.

Platform Abstraction
^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Linux Platform Support
   :id: comp_req__time_daemon__platform_linux
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall support Linux platforms for shared memory access and IPC communication.

.. comp_req:: QNX Platform Support
   :id: comp_req__time_daemon__platform_qnx
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The time_daemon component shall support QNX 8.0 SDP platforms for shared memory access and IPC communication.

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
