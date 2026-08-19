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

.. _time_slave:

Time Slave
##########

.. document:: Time Slave
   :id: doc__time_slave
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__cmpt_request
   :tags: time_slave

.. comp:: Time Slave
   :id: comp__time_slave
   :security: NO
   :safety: QM
   :status: valid
   :belongs_to: feat__time[version==1]


Abstract
========

This component implements a gPTP (IEEE 802.1AS) time synchronization slave daemon that receives time synchronization data from network and publishes it to IPC for client applications.

Specification
=============

The component provides gPTP slave functionality with network message processing and IPC publishing:

* :need:`comp_req__time_slave__gptp_slave`
* :need:`comp_req__time_slave__time_publishing`
* :need:`comp_req__time_slave__sync_state`
* :need:`comp_req__time_slave__phc_sync`
* :need:`comp_req__time_slave__error_reporting`
* :need:`comp_req__time_slave__efficient_access`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   detailed_design/index
   requirements/index
