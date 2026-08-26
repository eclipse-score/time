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

Basic Clock Examples
====================

Overview
--------

Three examples demonstrate the basic SCORE clock types: ``system_time``, ``steady_time``,
and ``high_res_steady_time``. All follow an identical pattern - a periodic time printer
that outputs time values once per second until interrupted.

These examples show the fundamental pattern for using SCORE time APIs and can serve as
starting points for applications requiring simple time reading.

Common Implementation Pattern
-----------------------------

All three examples share the same structure:

**Handler Class**
  Wrapper around ``Clock<Tag>::GetInstance()`` that provides a clean ``GetCurrentTime()``
  method returning a ``TimeReport`` struct.

**Main Program**
  - Signal handling for graceful shutdown (SIGINT/SIGTERM)
  - Loop reading time every second
  - Simple text output with sequence numbers
  - Consistent error handling

**Unit Tests**
  Demonstrate mocking with ``ScopedClockOverride`` for dependency injection.

Building and Running
--------------------

.. code-block:: bash

   # Build any of the basic examples
   bazel build //examples/time/system_time
   bazel build //examples/time/steady_time
   bazel build //examples/time/high_res_steady_time

   # Run examples
   bazel run //examples/time/system_time
   bazel run //examples/time/steady_time
   bazel run //examples/time/high_res_steady_time

   # Run tests
   bazel test //examples/time/system_time/src:system_time_handler_test
   bazel test //examples/time/steady_time/src:steady_time_handler_test
   bazel test //examples/time/high_res_steady_time/src:high_res_steady_time_handler_test

Example Output
--------------

Each example prints time in a similar format:

**System Time:**

.. code-block:: text

   SystemTime printer started. Press Ctrl+C to stop.
   [0]  unix=1720184400.123456789 s
   [1]  unix=1720184401.234567890 s
   [2]  unix=1720184402.345678901 s
   ...

**Steady Time:**

.. code-block:: text

   SteadyTime printer started. Press Ctrl+C to stop.
   [0]  monotonic=12345.123456789 s
   [1]  monotonic=12346.234567890 s
   [2]  monotonic=12347.345678901 s
   ...

**High-Resolution Steady Time:**

.. code-block:: text

   HighResSteadyTime printer started. Press Ctrl+C to stop.
   [0]  time=12345.123456789 s
   [1]  time=12346.234567890 s
   [2]  time=12347.345678901 s
   ...

Code Structure
--------------

Each example follows this pattern:

**Handler Header** (``*_time_handler.h``):

.. code-block:: cpp

   struct TimeReport {
       std::int64_t time_field_ns{0};  // Field name varies by clock type
   };

   class TimeHandler {
   public:
       TimeReport GetCurrentTime() const noexcept {
           const auto snapshot = ClockType::GetInstance().Now();
           return TimeReport{snapshot.TimePointNs().count()};
       }
   };

**Main Program** (``main.cpp``):

.. code-block:: cpp

   volatile std::sig_atomic_t gShutdownRequested{0};
   extern "C" void HandleSignal(int) noexcept { gShutdownRequested = 1; }

   int main() {
       signal(SIGINT, HandleSignal);
       signal(SIGTERM, HandleSignal);

       HandlerType handler;
       std::uint64_t seq{0};

       while (gShutdownRequested == 0) {
           const auto report = handler.GetCurrentTime();
           PrintReport(report, seq++);
           std::this_thread::sleep_for(std::chrono::seconds{1});
       }
       return 0;
   }

Testing Pattern
---------------

All examples use the same mocking approach:

.. code-block:: cpp

   TEST(HandlerTest, GetCurrentTime) {
       auto mock = std::make_shared<ClockBackendMock>();
       score::time::test_utils::ScopedClockOverride<ClockTag> guard{mock};

       EXPECT_CALL(*mock, Now()).WillOnce(Return(test_snapshot));

       HandlerType handler;
       const auto report = handler.GetCurrentTime();

       EXPECT_EQ(expected_value, report.time_field_ns);
   }

.. note::

   Tests using ``ScopedClockOverride`` must declare ``tags = ["exclusive", "unit"]``
   in their Bazel BUILD file to prevent parallel execution conflicts.

Bazel Build Setup
-----------------

Understanding the dependency structure helps when adapting these examples for your application.

Target Structure
~~~~~~~~~~~~~~~~

Each example has three Bazel targets in ``examples/time/<clock_type>/src/BUILD``:

.. code-block:: python

   cc_library(
       name = "time_handler",
       hdrs = ["system_time_handler.h"],
       deps = ["//score/time/system_time:interface"],  # Header-only dep
   )

   cc_binary(
       name = "system_time",
       srcs = ["main.cpp"],
       deps = [
           ":time_handler",
           "//score/time/system_time",  # Production backend
       ],
   )

   cc_test(
       name = "system_time_handler_test",
       srcs = ["system_time_handler_test.cpp"],
       tags = ["exclusive", "unit"],  # Required for ScopedClockOverride
       deps = [
           ":time_handler",
           "//score/time/system_time:system_time_mock",  # Mock backend
           "@googletest//:gtest",
           "@googletest//:gtest_main",
       ],
   )

Dependency Layers
~~~~~~~~~~~~~~~~~

**Handler Library** (``time_handler``):
  - Header-only wrapper around SCORE clock API
  - Depends on ``:interface`` target (types only, no implementation)
  - Can be tested without linking production backend

**Binary** (``system_time``, ``steady_time``, ``high_res_steady_time``):
  - Links production backend (``//score/time/<type>``)
  - Depends on handler library
  - Minimal dependencies for deployment

**Test** (``*_handler_test``):
  - Links mock backend (``//score/time/<type>:*_mock``)
  - Uses ``ScopedClockOverride`` for dependency injection
  - **Must** have ``tags = ["exclusive", "unit"]`` to prevent parallel test conflicts

Key Dependency Targets
~~~~~~~~~~~~~~~~~~~~~~

For each clock type (``system_time``, ``steady_time``, ``high_res_steady_time``):

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Target
     - Purpose
   * - ``//score/time/<type>:interface``
     - Header-only, types and tag definitions
   * - ``//score/time/<type>``
     - Production backend implementation
   * - ``//score/time/<type>:<type>_mock``
     - GMock test double for unit testing

Adapting Your Application
~~~~~~~~~~~~~~~~~~~~~~~~~

To use these patterns in your code:

1. **Production code** depends on ``:interface`` for headers, production target for binary:

   .. code-block:: python

      cc_library(
          name = "my_component",
          hdrs = ["my_component.h"],
          deps = ["//score/time/steady_time:interface"],
      )

      cc_binary(
          name = "my_app",
          deps = [
              ":my_component",
              "//score/time/steady_time",  # Link production backend
          ],
      )

2. **Tests** depend on ``:interface`` and ``*_mock``:

   .. code-block:: python

      cc_test(
          name = "my_component_test",
          tags = ["exclusive", "unit"],  # Required!
          deps = [
              ":my_component",
              "//score/time/steady_time:steady_time_mock",
              "@googletest//:gtest_main",
          ],
      )

This layering keeps compile times fast (interface-only deps) and enables testing without
runtime dependencies.
