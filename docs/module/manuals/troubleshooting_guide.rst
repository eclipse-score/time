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

.. _manual_time_troubleshooting:

*********************
Troubleshooting Guide
*********************

This guide provides solutions to common problems encountered when using or integrating the S-CORE ``time`` module.

Clock is Not Reliable or Not Available
======================================

**Symptom:**
Your application calls ``clock.Now()``, but ``snapshot.Status().IsReliable()`` always returns `false`. Or, ``clock.WaitUntilAvailable()`` runs into a timeout.

**Potential Causes and Solutions:**

1.  **TimeSlave Not Running or Not Synchronized:**
    *   **Check:** Is the `time_slave` process running on the ECU?
    *   **Check:** Is there a PTP Grandmaster Clock active on the network, in the same PTP domain as the `time_slave` (default domain: 0)?
    *   **Solution:** Ensure the `time_slave` is started correctly and that a PTP master is present and reachable on the specified network interface. Check the logs of the `time_slave` for messages related to master detection.

2.  **TimeDaemon Not Running:**
    *   **Check:** Is the `time_daemon` process running on the ECU? The `time_slave` can run, but if the `time_daemon` isn't there to process the data, client applications will not receive reliable time.
    *   **Solution:** Ensure the `time_daemon` process is started.

3.  **IPC Channel Mismatch:**
    *   **Check:** The `time_slave` and `time_daemon` communicate via a POSIX shared memory file. By default, this is ``/gptp_ptp_info``.
    *   **Solution:** Verify that this file exists in the shared memory file system (e.g., under `/dev/shm/` on Linux). Check for permission issues that might prevent one of the processes from accessing the file.

4.  **Sync Timeout:**
    *   **Check:** The `time_slave` has a built-in timeout (`sync_timeout_ms`, default: 3300 ms). If it doesn't receive PTP Sync messages within this period, it declares a timeout.
    *   **Solution:** Check the network for packet loss. If you are in a simulated environment (QEMU, Docker), ensure the virtual network bridge is configured correctly.

"Permission Denied" on TimeSlave Startup
========================================

**Symptom:**
The `time_slave` process fails to start with an error message similar to "Permission denied", "Operation not permitted", or a socket creation error.

**Cause & Solution:**

The `time_slave` requires elevated network privileges to open a raw PTP socket on the specified network interface.

*   **On Linux:** Grant the ``CAP_NET_RAW`` capability to the ``time_slave`` binary instead of running it as root:

    .. code-block:: bash

        sudo setcap cap_net_raw+ep /path/to/time_slave

*   **On QNX:** The ``time_slave`` opens ``/dev/bpf`` (Berkeley Packet Filter device) to capture raw PTP frames. Ensure the process user has read/write permission on ``/dev/bpf``. If a PHC device is configured (``phc_device`` option), the process also needs read/write access to that device node.
*   **Shared memory access:** If the error refers to ``/gptp_ptp_info``, verify that the user running ``time_slave`` has read/write permission on the shared memory path (``/dev/shm/`` on Linux). Adjust the file permissions or run both ``time_slave`` and ``time_daemon`` under the same user/group.

Understanding Log Messages
==========================

The `time` module components use specific logging contexts to identify the source of a message. This can help you pinpoint where a problem is occurring.

.. list-table:: Logging Contexts
   :widths: 20 80
   :header-rows: 1

   * - Context ID
     - Description
   * - ``[TSAP]``
     - **Time Slave Application.** Relates to the main lifecycle (Initialize/Run) of the ``time_slave`` process.
   * - ``[GTPS]``
     - **GPTP Slave.** Relates to the core gPTP protocol engine within the ``time_slave`` (e.g., parsing PTP messages, state machines).
   * - ``[GPTP]``
     - **GPTP Machine Adapter.** Relates to the component within the ``time_daemon`` that receives and processes the data from shared memory.
