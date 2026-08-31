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

.. _ts_client:

Time Sync Client
################

.. document:: Time Sync Client
   :id: doc__ts_client
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__cmpt_request
   :tags: ts_client

.. comp:: Time Sync Client
   :id: comp__ts_client
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: feat__time

Abstract
========

This component provides IPC mechanisms for time synchronization data exchange between time daemons and client applications within an ECU.

Specification
=============

The component provides shared memory-based IPC for distributing time synchronization data with thread-safe, low-latency access:

* :need:`comp_req__ts_client__shared_memory_mgmt`
* :need:`comp_req__ts_client__shm_validation`
* :need:`comp_req__ts_client__publisher_creates`
* :need:`comp_req__ts_client__receiver_multi_reader`
* :need:`comp_req__ts_client__data_validity`
* :need:`comp_req__ts_client__seqlock_protocol`
* :need:`comp_req__ts_client__sync_status_data`
* :need:`comp_req__ts_client__sync_fup_data`
* :need:`comp_req__ts_client__pdelay_data`
* :need:`comp_req__ts_client__time_correlation_data`
* :need:`comp_req__ts_client__platform_linux`
* :need:`comp_req__ts_client__platform_qnx`
* :need:`comp_req__ts_client__error_reporting`
* :need:`comp_req__ts_client__cache_optimization`
* :need:`aou_req__ts_client__single_publisher`
* :need:`aou_req__ts_client__shm_permissions`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   detailed_design/index
   requirements/index
