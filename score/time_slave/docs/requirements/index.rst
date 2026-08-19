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

Component Time Slave Requirements
##################################

.. document:: Time Slave Requirements
   :id: doc__time_slave_requirements
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp[version==1]
   :tags: requirements, time_slave

Functional Requirements
-----------------------

.. comp_req:: gPTP Slave Protocol
   :id: comp_req__time_slave__gptp_slave
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall receive and process gPTP (IEEE 802.1AS) protocol messages as a slave endpoint when connected to a network with a gPTP master.

.. comp_req:: Time Data Publishing
   :id: comp_req__time_slave__time_publishing
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall publish synchronized time data to IPC after each successful synchronization update from the gPTP master.

.. comp_req:: Synchronization State Tracking
   :id: comp_req__time_slave__sync_state
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall track synchronization state and report loss of synchronization when no valid gPTP messages are received within the configured timeout period.

.. comp_req:: Hardware Clock Synchronization
   :id: comp_req__time_slave__phc_sync
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall support synchronization of the PTP hardware clock with the gPTP master when hardware timestamping is available.

.. comp_req:: Error Reporting
   :id: comp_req__time_slave__error_reporting
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall report initialization failures and protocol errors via the logging interface when they occur.

.. comp_req:: Lifecycle Management
   :id: comp_req__time_slave__lifecycle
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall support lifecycle management including initialization, runtime execution, and graceful shutdown.

.. comp_req:: Peer Delay Measurement
   :id: comp_req__time_slave__peer_delay
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall measure network peer delay via IEEE 802.1AS PDelayReq/PDelayResp exchange and include it in published time data.

.. comp_req:: Platform Portability
   :id: comp_req__time_slave__platform_portability
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall support deployment on multiple target platforms (Linux, QNX) with platform-specific network and hardware clock adaptations.

Non-Functional Requirements
---------------------------

.. comp_req:: Efficient Time Access
   :id: comp_req__time_slave__efficient_access
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The component shall publish time data via shared memory to enable fast client access without kernel calls.

Assumption of Use Requirements
------------------------------

.. aou_req:: Network Interface Availability
   :id: aou_req__time_slave__network_available
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The user shall ensure the configured network interface is available and supports hardware timestamping before starting the component.

.. aou_req:: Single Instance Per Interface
   :id: aou_req__time_slave__single_instance
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The user shall run only one time_slave instance per network interface to avoid protocol conflicts.

.. needextend:: is_external == False and "time_slave" in id
   :+tags: time_slave


.. toctree::
   :maxdepth: 1

   chklst_req_inspection
