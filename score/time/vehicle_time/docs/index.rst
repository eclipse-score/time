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

.. _vehicle_time:

VehicleTime
###########

.. note:: PTP-synchronized vehicle clock component

.. document:: VehicleTime
   :id: doc__vehicle_time
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__cmpt_request
   :tags: vehicle_time

Abstract
========

This component implements the PTP-synchronized vehicle clock facade.
``VehicleClock::Now`` returns a ``ClockSnapshot`` bundling the timepoint
with a ``VehicleTimeStatus`` (synchronization / leap flags, rate
deviation), and the component exposes ``Init`` / ``IsAvailable`` /
``WaitUntilAvailable`` because the underlying backend depends on an IPC
channel to the time master.

Specification
=============

The component provides a snapshot-with-status API and explicit
availability lifecycle:

* :need:`comp_req__vehicle_time__snapshot`
* :need:`comp_req__vehicle_time__lifecycle`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   requirements/index
   architecture/index
