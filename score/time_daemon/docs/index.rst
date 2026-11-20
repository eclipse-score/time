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

This component implements a time daemon that receives time synchronization data from its time data sources, performs verification and qualification, and provides validated time information to client applications.

Specification
=============

The time_daemon component acts as a verification layer between its time data sources and client applications, most notably the time component (:need:`comp__time`). It is designed to host one or more time bases. The time base currently provided is vehicle time, which receives gPTP time synchronization data from the time_slave component.

For each time base the component performs the following stages:

1. **Data Reception**: Receives time synchronization data from the time data source (for vehicle time: gPTP data from time_slave)
2. **Verification Pipeline**: Validates data through three checks:

   * Synchronization status validation
   * Timeout detection (no new time sync data within configured span of time)
   * Time jump detection (received master timestamp deviates from its expected value by more than the configured threshold)

3. **Provision**: Provides verified time data with quality indicators to client applications

Key Behaviors
-------------

**Startup Stabilization**: Synchronization state changes are not reported during a configurable span of time after initial synchronization to avoid spurious time jump detection.

**Error Recovery**: Time jump and timeout conditions are non-fatal. The component continues providing time data with appropriate status flags set. Time jump condition clears after configurable number of consecutive valid frames.

**Start Order Independence**: Time data sources do not need to be started before time_daemon. If a time data source is not yet available, time_daemon continues its own startup without blocking on it and retries establishing the connection until it succeeds or the startup timeout expires. If initialization of a time base does not complete within the startup timeout, time_daemon provides no time data and terminates.

**Multi-Client Support**: Multiple client applications can concurrently read the provided time data.

**Platform Support**: Linux and QNX 8.0 SDP platforms supported.

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
