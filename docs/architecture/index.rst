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

Architecture of Time Feature
============================

Overview
~~~~~~~~

The Time Feature offers applications access to multiple time bases ("clocks") via a unified API.
The main focus is providing access to synchronized time bases, like a vehicle-local time base and a vehicle-external global time base (UTC).
Besides that it offers access to local (not necessarily synchronized) clocks, like
* steady/monotonic clock
* system clock
* high resolution clock.
Those clocks are typically also accessible via existing interfaces defined by the C++ std library or POSIX library.
The reason to provide them via the unified API is to offer application an "easy to mock" interface.

The usage of the unified Time API shall be free from interference (FFI).
The synchronization status of the vehicle time shall be safety qualifiable - means, it is guaranteed to always reflect the actual sync state of that time base.

The basic architecture is shown in the diagram below.

.. raw:: html

   <div style="overflow-x: auto; max-width: 100%;">

.. uml:: _assets/sad_deployment.puml
   :alt: Raw architectural diagram

.. raw:: html

   </div>

Components decomposition
~~~~~~~~~~~~~~~~~~~~~~~~

The **gPTP Grand Master Clock** provides the vehicle time to all other devices in a vehicle via the PTP.
It is assumed to be a component running on a seprarte ECU.
It is therefore not part of the S-CORE Time Feature implementation.

The :doc:`**time_slave** component <../../score/time_slave/docs/index>` is responsible to do the gPTP-related protocol handling, i.e. to receive the Sync / Follow_up frames from the neighboured master and doing the path delay measuring.
The resulting data and timestamp information is forwarded via an IPC interface to the time_daemon component.

The :doc:`**time_daemon** component <../../score/time_daemon/docs/index>` is the heart of the Time Feature. It is responsible for getting the sync data received by the time_slave and determine the synchronization status of the current vehicle time.
The IPC interfaces to the adjacent components, time_slave and API libraries linked to the applications, need to be FFI to be able to guarantee the ASIL qualification of the synchronization status.

The :doc:`**time library** component <../../score/time_daemon/docs/index>` offers language specific APIs (currently C++ only, later Rust also) to the applications and handling the IPC with FFI to the time_daemon.
It is a library to be linked into the respective application processes.

The :doc:`**ts_client** component <../../score/ts_client/docs>` is an internal library abstracting the IPC between time_slave and time_daemon.
