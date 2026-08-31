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

.. _time_daemon:

Time Daemon
###########

.. document:: Time Daemon
   :id: doc__time_daemon
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__cmpt_request
   :tags: time_daemon

.. comp:: Time Daemon
   :id: comp__time_daemon
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: feat__time

Abstract
========

This component implements a time synchronization daemon that receives time data from time_slave via IPC, performs verification and qualification, and provides validated time information to applications.

Specification
=============

The component requirements are:

* :need:`comp_req__time_daemon__initialization`
* :need:`comp_req__time_daemon__shutdown`
* :need:`comp_req__time_daemon__gptp_shm_reception`
* :need:`comp_req__time_daemon__sync_validation`
* :need:`comp_req__time_daemon__sync_debounce`
* :need:`comp_req__time_daemon__time_jump_detection`
* :need:`comp_req__time_daemon__timeout_detection`
* :need:`comp_req__time_daemon__time_data_publishing`
* :need:`comp_req__time_daemon__published_data_content`
* :need:`comp_req__time_daemon__time_point_qualifier`
* :need:`comp_req__time_daemon__publish_interval`
* :need:`comp_req__time_daemon__periodic_fallback`
* :need:`comp_req__time_daemon__multi_client`
* :need:`comp_req__time_daemon__error_reporting`
* :need:`comp_req__time_daemon__time_jump_reaction`
* :need:`comp_req__time_daemon__time_jump_recovery`
* :need:`comp_req__time_daemon__timeout_reaction`
* :need:`comp_req__time_daemon__platform_linux`
* :need:`comp_req__time_daemon__platform_qnx`
* :need:`aou_req__time_daemon__gptp_shm_available`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   detailed_design/index
   requirements/index
   manuals/user_manual
