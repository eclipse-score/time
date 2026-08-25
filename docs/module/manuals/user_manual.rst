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

.. _user_manual:

User Manual
###########

.. document:: User Manual Time Module
   :id: doc__user_manual_time
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__training_path[version==1]

Overview
========

This user manual provides comprehensive guidance for integrating and deploying the S-CORE ``time`` module from a system integrator perspective.

The S-CORE ``time`` module provides a robust, high-precision time base for applications on an ECU,
synchronized to a network-wide PTP (Precision Time Protocol) Grandmaster Clock. The module consists of three components:

* **Client Library** (``score::time``): C++ API for accessing synchronized time
* **TimeSlave**: System daemon that synchronizes with the PTP Grandmaster over the network
* **TimeDaemon**: System daemon that provides quality-assured time to client applications

This module manual covers module-level integration, deployment, and troubleshooting. For component-specific usage and configuration, refer to the component manuals below.

For build and test of the module itself, please refer to the main documentation.

API Description
---------------

The primary interface for applications to access synchronized time is the ``score::time`` client library:

.. toctree::
   :maxdepth: 2

   api_description/api_usage
   api_description/lifecycle
   api_description/testing_guide

.. note::
   For a complete C++ API reference with full class and function documentation,
   please refer to the generated Doxygen documentation (to be added in future releases).

Choosing the Right Clock
=========================

The S-CORE ``time`` module provides several clock types, each designed for a specific use case. Understanding their differences is crucial for writing robust and correct applications.

Select clock type based on use case. No clock type is universally better; each has a different purpose.

.. list-table:: Clock Types Overview
   :widths: 20 40 40
   :header-rows: 1

   * - Clock Type
     - Key Characteristic
     - Typical Use Case
   * - ``VehicleTime``
     - High-accuracy, PTP-synchronized, quality-assured network time.
     - Cross-ECU correlation, synchronized logging, and decisions that depend on vehicle-wide time consistency (for example: validating whether a vehicle-time-stamped frame is too old and should be discarded).
   * - ``SystemTime``
     - The system's "wall clock" time (Unix time). Can jump forwards or backwards (e.g., due to NTP correction or manual changes).
     - Displaying human-readable timestamps. Creating log entries where absolute time is more important than monotonic progression.
   * - ``SteadyTime``
     - A clock that is guaranteed to only ever move forward (monotonic). Its starting point is arbitrary (e.g., system boot time).
     - Measuring time intervals, implementing timeouts, scheduling tasks where guaranteed monotonic progression is essential.
   * - ``HighResSteadyTime``
     - A monotonic clock that provides the highest possible resolution the underlying hardware can offer.
     - High-precision performance measurements and profiling, or very short-interval timing.

.. _component_manuals:

Component Manuals
-----------------

For detailed component-specific user manuals, see:

* :doc:`/time_slave/manuals/user_manual`
* :doc:`/time_daemon/manuals/user_manual`

Examples
--------

Practical examples and tutorials for using the time module:

.. toctree::
   :maxdepth: 2

   examples/index

Environment Needs
=================

Basic needed software environment for the module:

* **C++**: C++17 or later
* **Build System**: Bazel 6.0 or later
* **Operating Systems**: Linux, QNX

Dependencies
------------

* Standard library (STL/Core)
* PTP Grandmaster Clock (external network time source)
* POSIX shared memory support
* Network hardware with PHC (PTP Hardware Clock) support

See also MODULE.bazel files for more details on dependencies.

Performance Considerations
==========================

The ``time`` module is designed for high-performance, low-latency time access in automotive ECUs:

* **VehicleTime access**: Sub-microsecond latency via POSIX shared memory with seqlock
* **Lock-free IPC**: TimeDaemon reads from TimeSlave without blocking
* **Hardware clock sync**: Direct PHC adjustment for nanosecond-precision synchronization
* **Minimal overhead**: Singleton pattern, zero allocations in time-critical paths

For detailed performance analysis and benchmarks, this information will be added in future releases.

Integration Guidelines
======================

Integrating with Your Project
------------------------------

1. Add the module to your Bazel workspace:

   .. code-block:: python

      # In your MODULE.bazel
      bazel_dep(name = "score_time", version = "1.0")

2. Reference in your build files:

   .. code-block:: python

      cc_library(
          name = "my_target",
          deps = [
                "@score_time//score/time/vehicle_time:vehicle_time",  # For VehicleTime
                # OR
                "@score_time//score/time/system_time:system_time",     # For SystemTime
                # OR
                "@score_time//score/time/steady_time:steady_time",     # For SteadyTime
                # OR
                "@score_time//score/time/high_res_steady_time:high_res_steady_time",  # For HighResSteadyTime
          ],
      )

3. Include headers and use the API in your code:

   .. code-block:: cpp

      #include "score/time/clock.h"
      #include "score/time/vehicle_time.h"

      auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();
      const auto snapshot = clock.Now();
      if (snapshot.Status().IsReliable())
      {
          // Safe to use snapshot.TimePoint()
      }

For component tests, use the mock variants where needed, for example:

.. code-block:: python

   cc_test(
       name = "my_test",
       deps = [
           "@score_time//score/time/vehicle_time:vehicle_time_mock",
       ],
   )

Runtime Requirements
--------------------

If your application uses ``VehicleTime``, both ``TimeSlave`` and ``TimeDaemon`` services must be running.

``SystemTime``, ``SteadyTime``, and ``HighResSteadyTime`` do not depend on these daemons.

For service deployment and configuration details, refer to:

* :doc:`/time_slave/manuals/user_manual`
* :doc:`/time_daemon/manuals/user_manual`

System Services Deployment
---------------------------

The ``time`` module requires two system daemons to be running. These processes must be managed by the system's service manager (e.g., `systemd` on Linux, or a launch script on QNX).

.. For detailed configuration of each daemon (OS privileges, network configuration, command-line arguments), refer to the  :ref:`component_manuals<component manuals>` linked above.

Troubleshooting
===============

For troubleshooting tips refer to :doc:`troubleshooting_guide`.

Safety and Security
===================

**Safety Classification**: ASIL-B (TBC)

Safety classification details are currently being aligned with ongoing stakeholder and feature requirement clarifications. Current working classification is:

* ``score::time`` library: ASIL-B (TBC)
* ``TimeDaemon``: ASIL-B
* ``TimeSlave``: QM

For final safety-critical usage requirements and guidelines, refer to the safety manual updates in upcoming releases.

**Security Considerations**:

* The ``time`` module assumes a trusted network for PTP communication
* No authentication or encryption is provided for PTP messages (per IEEE 1588 standard)
* OS-level security controls limit attack surface for TimeSlave daemon (Linux Capabilities on Linux, equivalent least-privilege process configuration on QNX)

License
=======

This module is licensed under the Apache License Version 2.0.
See the LICENSE file in the repository for full license text.

Feedback and Contributions
==========================

Your feedback and contributions are welcome! Please report issues or suggestions through the
project's issue tracker or contribute directly to the repository.
