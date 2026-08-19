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

.. comp_req:: IPC Time Data Reception
   :id: comp_req__time_daemon__ipc_reception
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall receive time synchronization data from time_slave via the shared memory IPC channel.

.. comp_req:: Synchronization Status Validation
   :id: comp_req__time_daemon__sync_validation
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall validate synchronization status and provide accuracy qualifier information to indicate time base quality.

.. comp_req:: Time Jump Detection
   :id: comp_req__time_daemon__time_jump_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_acc_qual_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall detect time jumps to the future or past and report these conditions via status flags.

.. comp_req:: Timeout Detection
   :id: comp_req__time_daemon__timeout_detection
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall detect timeout conditions when no valid time data is received within a configured period.

.. comp_req:: Error Reporting
   :id: comp_req__time_daemon__error_reporting
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync_log[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall report verification failures and operational errors via the logging interface.

.. comp_req:: Lifecycle Management
   :id: comp_req__time_daemon__lifecycle
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall support lifecycle management including initialization, runtime execution, and graceful shutdown.

.. comp_req:: Consistent Publishing Rate
   :id: comp_req__time_daemon__consistent_rate
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall maintain consistent time data publishing rates to client applications regardless of upstream time source availability or delays.

.. comp_req:: Multi-Client Support
   :id: comp_req__time_daemon__multi_client
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_daemon

   The component shall support concurrent access from multiple client applications without degradation of time accuracy or availability.

.. needextend:: is_external == False and "time_daemon" in id
   :+tags: time_daemon

.. toctree::
   :maxdepth: 1

   chklst_req_inspection
