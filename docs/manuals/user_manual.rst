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
   :safety: ASIL-B (TBC)
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

.. _component_manuals:

Component Manuals
-----------------

For detailed component-specific user manuals:

.. toctree::
   :maxdepth: 1

   /components/time/manuals/user_manual
   /components/time_slave/manuals/user_manual
   /components/time_daemon/manuals/user_manual

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
          deps = ["@score_time//score/time/vehicle_time:vehicle_time"],
      )

3. Include headers and compile your code

System Services Deployment
---------------------------

The ``time`` module requires two system daemons to be running. These processes must be managed by the system's service manager (e.g., `systemd` on Linux, or a launch script on QNX).

.. For detailed configuration of each daemon (OS privileges, network configuration, command-line arguments), refer to the  :ref:`component_manuals<component manuals>` linked above.

Version History, Compatibility, and Troubleshooting
===================================================

For comprehensive information on the following topics:

* Version history and changes
* Compatibility notes and upgrade instructions
* Known issues and limitations
* Troubleshooting tips and solutions
* Security vulnerabilities (CVEs)

.. toctree::
   :maxdepth: 1

   troubleshooting_guide

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
