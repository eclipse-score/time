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

.. _time_daemon_user_manual:

Time Daemon User Manual
#######################

.. document:: User Manual Time Daemon Component
   :id: doc__user_manual_time_daemon
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__training_path[version==1]

Overview
========

The ``TimeDaemon`` component is a system daemon responsible for quality assurance and providing synchronized time to local applications on the ECU. It reads raw synchronization data from shared memory (published by ``TimeSlave``), performs quality checks and plausibility assessments, and provides the final ``score::time`` API to client applications.

For module-level integration and deployment information, see the main module manual.

Configuration
=============

.. toctree::
   :maxdepth: 2

   config/configuration_guide

Runtime Requirements
====================

The ``TimeDaemon`` requires:

* ``TimeSlave`` must be running and publishing data to shared memory
* Access to POSIX shared memory segment (``/gptp_ptp_info``)
* Managed by system service manager (e.g., `systemd` on Linux, launch script on QNX)
