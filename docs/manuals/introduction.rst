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

.. _manual_time_introduction

Architecture and Components
===========================

The sCore ``time`` module provides a robust, high-precision time base for applications on an ECU, synchronized to a network-wide PTP (Precision Time Protocol) Grandmaster Clock. The architecture is split into two main processes, the **TimeSlave** and the **TimeDaemon**, which communicate via a highly efficient shared memory channel.

.. figure:: /docs/features/time_slave/_assets/timeslave_deployment.png
   :align: center
   :alt: TimeSlave Deployment View

   The deployment view shows the key components and their interactions.

Component Overview
------------------

**1. Time Master (External)**

*   The **PTP Grandmaster Clock** is the authoritative time source for the entire vehicle network. It periodically sends PTP synchronization messages (EtherType ``0x88F7``) over the Ethernet network.

**2. TimeSlave Process**

*   The ``TimeSlave`` is a standalone process responsible for all network-related PTP activities. It is the direct counterpart to the Time Master.
*   **GptpEngine**: The core of the TimeSlave. It runs two main threads:
    *   **RxThread**: Listens for incoming PTP messages from the Time Master.
    *   **PdelayThread**: Actively measures the network latency (path delay) to the communication partner, as specified by the gPTP standard.
*   **PhcAdjuster**: Receives the calculated time offset and frequency deviation from the ``GptpEngine``. It then directly adjusts the hardware clock of the network card (PHC Device) using kernel system calls like ``clock_adjtime``. This ensures the hardware clock is precisely synchronized.
*   **GptpIpcPublisher**: Publishes the raw synchronization data, including timestamps and quality metrics, into a POSIX shared memory segment (``/gptp_ptp_info``).
*   **ProbeManager + Recorder**: An instrumentation component for diagnostics and performance monitoring.

**3. TimeDaemon Process**

*   The ``TimeDaemon`` is responsible for quality assurance and providing the synchronized time to all local applications on the ECU. It acts as the server for the sCore time service.
*   **GptpIpcReceiver**: Reads the raw data from the shared memory segment published by the ``TimeSlave``.
*   **ShmPTPEngine**: The core of the TimeDaemon. It wraps the ``GptpIpcReceiver``, processes the raw ``GptpIpcData``, performs quality checks and plausibility assessments, and converts it into the final, high-level ``PtpTimeInfo`` format that client applications will consume.

**4. Shared Memory (IPC Channel)**

*   A POSIX shared memory segment, typically ``/gptp_ptp_info``, serves as a high-performance, lock-free communication channel between the ``TimeSlave`` and the ``TimeDaemon``.
*   **seqlock**: The communication is protected by a seqlock (sequence lock) mechanism. This allows the ``GptpIpcReceiver`` to read the data without ever being blocked, ensuring real-time safety, while also guaranteeing that it never receives partially updated (torn) data.

Data Flow Summary
-----------------

1.  The **Time Master** sends PTP messages over the Ethernet network.
2.  The **GptpEngine** in the ``TimeSlave`` process receives these messages.
3.  The ``GptpEngine`` calculates the time offset and adjusts the **PHC Device** (hardware clock) via the ``PhcAdjuster``.
4.  Simultaneously, the ``GptpEngine`` passes the raw synchronization data to the **GptpIpcPublisher**.
5.  The publisher writes the data into **Shared Memory** using a seqlock.
6.  The **GptpIpcReceiver** in the ``TimeDaemon`` process reads the data from shared memory, also using the seqlock mechanism.
7.  The **ShmPTPEngine** processes this data, turning it into the final, quality-assured time base for the system.

Choosing the Right Clock
========================

The sCore ``time`` module provides several clock types, each designed for a specific use case. Understanding their differences is crucial for writing robust and correct applications.

In general, you should **always prefer ``VehicleTime``** unless you have a specific reason to measure a local time interval or need a simple wall-clock timestamp for purely informational purposes.

.. list-table:: Clock Types Overview
   :widths: 20 40 40
   :header-rows: 1

   * - Clock Type
     - Key Characteristic
     - Typical Use Case
   * - ``VehicleTime``
     - High-precision, PTP-synchronized, quality-assured network time. **This is the recommended clock for almost all applications.**
     - Synchronized logging across ECUs, event timestamping, any logic that depends on a common time base in the vehicle.
   * - ``SystemTime``
     - The system's "wall clock" time (Unix time). Can jump forwards or backwards (e.g., due to NTP correction or manual changes).
     - Displaying human-readable timestamps. Creating log entries where absolute time is more important than monotonic progression.
   * - ``SteadyTime``
     - A clock that is guaranteed to only ever move forward (monotonic). Its starting point is arbitrary (e.g., system boot time).
     - Measuring time intervals, implementing timeouts, scheduling tasks where guaranteed monotonic progression is essential.
   * - ``HighResSteadyTime``
     - A monotonic clock that provides the highest possible resolution the underlying hardware can offer.
     - High-precision performance measurements and profiling, or very short-interval timing.
