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

.. _manual_time_integration:

*****************
Integration Guide
*****************

This guide is intended for system integrators who are responsible for deploying and configuring the ``time`` module services on an ECU.

System Services
===============

The S-CORE ``time`` module consists of two essential system services that must be running for the client library to function:

1.  ``time_slave``: The PTP slave process that communicates with the network master clock.
2.  ``time_daemon``: The daemon that processes the data from the ``time_slave`` and provides it to client applications.

These two processes must be managed by the system's service manager (e.g., `systemd` on Linux, or a launch script on QNX).

Runtime Requirements
====================

Operating System Privileges
---------------------------
The ``time_slave`` process requires elevated privileges to access raw network sockets and control the hardware clock. It is strongly recommended **not** to run this process as the `root` user. Instead, grant the required Linux Capabilities to the executable:

.. code-block:: bash

   sudo setcap cap_net_admin,cap_net_raw,cap_sys_time+eip /path/to/your/time_slave

*   ``cap_net_admin``: For network interface configuration.
*   ``cap_net_raw``: For the use of raw sockets to listen to PTP traffic.
*   ``cap_sys_time``: For adjusting the system's hardware clock.

Network Configuration
---------------------
*   The network interface used for PTP communication **must** be provided to the ``time_slave`` via the ``-i, --interface <name>`` command-line argument.
*   The ECU must have network connectivity to the PTP Grandmaster clock on this interface.

Build-Time Dependencies (Bazel)
-------------------------------
For an application to successfully link against the ``score::time`` client library, its ``cc_binary`` or ``cc_library`` target in the `BUILD` file must include the `vehicle_time` dependencies.

Please refer to the **Bazel Build Setup** section in the examples documentation for a detailed explanation of the required ``deps``.
