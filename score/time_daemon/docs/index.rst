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
   :belongs_to: comp__time_daemon
   :tags: time_daemon

.. comp:: Time Daemon
   :id: comp__time_daemon
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: feat__time[version==1]

Abstract
========

This component implements a time synchronization daemon that receives time data from time_slave via IPC, performs verification and qualification, and provides validated time information to applications.

Specification
=============

The time_daemon component acts as a verification and publishing layer between the time_slave component and client applications, most notably the time component (:need:`comp__time`), which reads the provided time data via the ``svt_receiver`` interface. It receives gPTP time synchronization data from shared memory, validates it through multiple verification stages, and publishes qualified time data via IPC.

The component operates as a continuous loop with the following stages:

1. **Data Reception**: Reads gPTP time synchronization data from shared memory written by time_slave
2. **Verification Pipeline**: Validates data through three checks:

   * Synchronization status validation
   * Timeout detection (no new time sync data within configured span of time)
   * Time jump detection (received master timestamp deviates from its expected value by more than the configured threshold)

3. **Publishing**: Publishes verified time data with quality indicators to clients via VehicleTime IPC interface

Key Behaviors
-------------

**Startup Stabilization**: Synchronization state changes are not reported during a configurable span of time after initial synchronization to avoid spurious time jump detection.

**Error Recovery**: Time jump and timeout conditions are non-fatal. The component continues providing time data with appropriate status flags set. Time jump condition clears after configurable number of  consecutive valid frames.

**Start Order Independence**: time_slave does not need to be started before time_daemon. If time_slave is not yet available, time_daemon continues its own startup and retries establishing the synchronization channel until it succeeds or the startup timeout expires.

**Multi-Client Support**: Multiple client applications can concurrently read the provided time data.

**Platform Support**: Linux and QNX 8.0 SDP platforms supported for shared memory and IPC operations.

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
