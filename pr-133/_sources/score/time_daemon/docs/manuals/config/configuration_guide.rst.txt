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

.. _time_daemon_configuration:

TimeDaemon Configuration
=========================

The ``TimeDaemon`` process currently operates **without any external configuration**. It relies on default, built-in settings for IPC communication.

Shared Memory Configuration
----------------------------

The daemon reads from the shared memory segment published by ``TimeSlave``:

* **Shared memory path**: ``/gptp_ptp_info``
* **IPC mechanism**: POSIX shared memory with seqlock protection

No runtime configuration options are exposed at this time. All settings are compiled into the binary.
