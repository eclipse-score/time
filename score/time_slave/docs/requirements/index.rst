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

Initialization and Lifecycle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Component Initialization
   :id: comp_req__time_slave__initialization
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall initialize the gPTP engine and IPC publisher, bind to the configured network interface, and attempt to enable hardware timestamping on the network interface during initialization.

.. comp_req:: Component Shutdown
   :id: comp_req__time_slave__shutdown
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall stop message processing threads, close network sockets, and release IPC resources when deinitialized or when a stop signal is received.

.. comp_req:: Domain Number Filtering
   :id: comp_req__time_slave__domain_filtering
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall filter gPTP messages by domain number (0-127 per IEEE 802.1AS) and process only messages matching the configured domain.

gPTP Protocol Message Processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Sync Message Reception
   :id: comp_req__time_slave__sync_reception
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall receive IEEE 802.1AS Sync messages from the network Grand Master and extract the receive timestamp using hardware timestamping when available, otherwise using software timestamping as a fallback.

.. comp_req:: Follow_Up Message Processing
   :id: comp_req__time_slave__followup_processing
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall process IEEE 802.1AS Follow_Up messages, match them to the corresponding Sync message by sequence ID, and extract the precise origin timestamp.

.. comp_req:: Offset Calculation
   :id: comp_req__time_slave__offset_calculation
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall compute the clock offset as the difference between the local receive timestamp and the Grand Master origin timestamp, accounting for the correction fields from both the Sync and Follow_Up messages per IEEE 802.1AS.

.. comp_req:: PDelayReq Transmission
   :id: comp_req__time_slave__pdelay_req
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall transmit IEEE 802.1AS PDelayReq messages at the configured interval (configurable, default 1000 milliseconds) and record the transmit timestamp.

.. comp_req:: Peer Delay Computation
   :id: comp_req__time_slave__pdelay_computation
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall compute network peer delay from PDelayResp and PDelayRespFollowUp timestamps using the IEEE 802.1AS peer delay formula: ((t2 - t1) + (t4 - t3c)) / 2, where t3c is the response origin timestamp corrected by the sum of the PDelayResp and PDelayRespFollowUp correction fields.

Clock Synchronization
^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: PHC Offset Adjustment
   :id: comp_req__time_slave__phc_offset
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall step-adjust the PTP hardware clock offset when the absolute value of the computed offset exceeds the configured step threshold (configurable, default 1 second).

.. comp_req:: PHC Frequency Adjustment
   :id: comp_req__time_slave__phc_frequency
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall slew-adjust the PTP hardware clock frequency based on the neighbor rate ratio computed from consecutive Sync/Follow_Up pairs with positive time intervals per IEEE 802.1AS Clause 11.4.1.

Status and Fault Detection
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Synchronization Timeout Detection
   :id: comp_req__time_slave__sync_timeout
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall set the timeout status flag when no Sync/Follow_Up message pair is successfully processed within the configured timeout period (configurable, range 100 milliseconds to 10 seconds).

.. comp_req:: Time Leap Future Detection
   :id: comp_req__time_slave__leap_future
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall set the time leap future flag when the master time increases by more than the configured future leap threshold between consecutive Sync/Follow_Up pairs. The future leap threshold shall be configurable in the range 1 millisecond to 60 seconds, with validation enforced during configuration loading.

.. comp_req:: Time Leap Past Detection
   :id: comp_req__time_slave__leap_past
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall set the time leap past flag when the master time decreases between consecutive Sync/Follow_Up pairs.

Data Publishing
^^^^^^^^^^^^^^^

.. comp_req:: Time Sync Data Publishing
   :id: comp_req__time_slave__sync_publishing
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall publish synchronized time data (PTP assumed time, offset correction, rate deviation, peer delay, status flags) via the ts_client IPC interface after processing each Sync/Follow_Up message pair.

.. comp_req:: Publish Interval
   :id: comp_req__time_slave__publish_interval
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_ctrl_flow[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall update the published time snapshot at a fixed interval of 50 milliseconds to provide consistent read latency for VehicleTime clients.

Platform Abstraction
^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Linux Platform Support
   :id: comp_req__time_slave__platform_linux
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall support Linux platforms for Ethernet frame transmission/reception with hardware timestamping and PTP hardware clock control.

.. comp_req:: QNX Platform Support
   :id: comp_req__time_slave__platform_qnx
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall support QNX 8.0 SDP platforms for Ethernet frame transmission/reception with hardware timestamping and PTP hardware clock control.

.. comp_req:: Hardware Timestamping
   :id: comp_req__time_slave__hw_timestamping
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall attempt to enable hardware timestamping on the network interface during initialization. If the platform does not support hardware timestamping capabilities (detected by ioctl or equivalent platform API failure), The time_slave component shall fall back to software timestamps and log a warning.

Error Handling
^^^^^^^^^^^^^^

.. comp_req:: Error Reporting
   :id: comp_req__time_slave__error_reporting
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync_log[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall log messages via the score::mw::log interface when initialization fails (error level), network operations fail (error level), protocol errors occur (error level), or hardware timestamping is unavailable (warning level).

Diagnostics and Debugging
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. comp_req:: Synchronization Diagnostics
   :id: comp_req__time_slave__diagnostics
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__time__vehicle_time_sync_log[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__time_slave

   The time_slave component shall provide optional runtime instrumentation to record gPTP synchronization events (Sync reception, peer delay completion, offset thresholds, time leaps) to a CSV file for debugging and diagnostics when enabled via the GptpEngineOptions diagnostics configuration parameter.

Assumption of Use Requirements
------------------------------

.. aou_req:: Network Interface Configuration
   :id: aou_req__time_slave__network_config
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The user shall configure a network interface and ensure the interface has an assigned link-layer address and the link is operationally up (IFF_UP and IFF_RUNNING flags set) before starting time_slave.

.. aou_req:: Single Instance per Interface
   :id: aou_req__time_slave__single_instance
   :reqtype: Process
   :security: NO
   :safety: QM
   :status: valid
   :version: 1

   The user shall run exactly one time_slave instance per network interface to prevent gPTP protocol conflicts. Multiple instances binding to the same interface will cause raw socket binding failures or multicast group membership conflicts.

.. needextend:: is_external == False and "time_slave" in id
   :+tags: time_slave


.. toctree::
   :maxdepth: 1

   chklst_req_inspection
