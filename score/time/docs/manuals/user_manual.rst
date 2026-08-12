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

.. _time_component_user_manual:

Time Library User Manual
########################

.. document:: User Manual Time Library Component
   :id: doc__user_manual_time_lib
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__training_path[version==1]

Overview
========

This user manual covers the ``score::time`` client library - the C++ API for accessing synchronized time in your applications.

The library provides multiple clock types (``VehicleTime``, ``SystemTime``, ``SteadyTime``, ``HighResSteadyTime``) with a unified interface for time access, lifecycle management, and testing.

For module-level integration and deployment information, see the main module manual.

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
     - High-precision, PTP-synchronized, quality-assured network time.
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

API Usage
=========

This section covers how to use the ``score::time`` client library in your applications:

.. toctree::
   :maxdepth: 2

   api_description/lifecycle
   api_description/testing_guide

.. note::
   For a complete C++ API reference with full class and function documentation,
   please refer to the generated Doxygen documentation (to be added in future releases).

Build Integration
=================

To use the ``score::time`` library in your application:

1. Add the module to your Bazel workspace:

   .. code-block:: python

      # In your MODULE.bazel
      bazel_dep(name = "score_time", version = "1.0")

2. Reference the clock type you need in your build files:

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

   For testing, use the mock variants:

   .. code-block:: python

      cc_test(
          name = "my_test",
          deps = [
              "@score_time//score/time/vehicle_time:vehicle_time_mock",
          ],
      )

3. Include headers in your code:

   .. code-block:: cpp

      #include "score/time/clock.h"
      #include "score/time/vehicle_time.h"

      // Example usage
      auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();
      auto snapshot = clock.Now();
      if (snapshot.Status().IsReliable()) {
          // Use snapshot.TimePoint()
      }

Runtime Requirements
====================

If using ``VehicleTime``, ``TimeSlave`` and ``TimeDaemon`` system services must be running.
``SystemTime``, ``SteadyTime``, and ``HighResSteadyTime`` do not depend on those daemons.
For deployment and configuration of these services, refer to the module manual and component manuals for
:doc:`/components/time_slave/manuals/user_manual` and :doc:`/components/time_daemon/manuals/user_manual`.
