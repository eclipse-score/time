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

The component provides time data reception, verification, and qualification:

* :need:`comp_req__time_daemon__ipc_reception`
* :need:`comp_req__time_daemon__sync_validation`
* :need:`comp_req__time_daemon__time_jump_detection`
* :need:`comp_req__time_daemon__timeout_detection`
* :need:`comp_req__time_daemon__error_reporting`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   detailed_design/index
   requirements/index
