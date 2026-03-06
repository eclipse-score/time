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

.. _inc_time_stakeholder_requirements:

########################
Stakeholder Requirements
########################

Vehicle Time Synchronization
=============================

.. stkh_req:: Vehicle clock synchronization via gPTP
   :id: stkh_req__time__vehicle_time_sync
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :rationale: Automotive applications require a common time reference locked to the in-vehicle network Time Master for sensor fusion, event correlation, and log timestamping.

   The system shall provide a vehicle-wide synchronized clock that is locked
   to the gPTP (IEEE 802.1AS) Time Master present on the vehicle Ethernet
   network.

.. stkh_req:: Multi-client time access without contention
   :id: stkh_req__time__multi_client_access
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :rationale: Multiple ECU applications need concurrent read access to the synchronized time without incurring scheduler latency or mutual-exclusion overhead.

   The system shall allow multiple concurrent reader applications to access
   the synchronized time with bounded, low latency and without blocking the
   time publisher.

.. stkh_req:: Absolute UTC time provision
   :id: stkh_req__time__absolute_time
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :rationale: Applications such as event recorders and over-the-air update clients require a wall-clock reference independent of the vehicle boot time.

   The system shall provide an absolute UTC time estimate derived from NTP
   or an external time source (e.g., GNSS), together with an estimated
   inaccuracy and source trustworthiness qualifier.

.. stkh_req:: Synchronization accuracy reporting
   :id: stkh_req__time__accuracy_reporting
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :rationale: Consumers of synchronized time must be able to detect degraded or unavailable synchronization and adapt their behavior accordingly.

   The system shall continuously report the synchronization quality of the
   vehicle clock (not synchronized, synchronized, unstable, time jump
   detected) and the ASIL integrity level of the published time value.

.. stkh_req:: Linux and QNX platform support
   :id: stkh_req__time__platform_support
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Non-Functional
   :rationale: The S-CORE platform targets both Linux-based and QNX-based automotive ECUs.

   The system shall operate on both Linux and QNX operating systems without
   changes to the application-level API.

.. stkh_req:: Synchronization diagnostic logging
   :id: stkh_req__time__diagnostics
   :status: valid
   :safety: QM
   :security: NO
   :reqtype: Functional
   :rationale: Field diagnostics and post-mortem analysis require a history of synchronization events accessible without additional IPC.

   The system shall maintain a circular event log of synchronization state
   changes, offset measurements, and peer delay samples readable by client
   applications via the shared memory region.
