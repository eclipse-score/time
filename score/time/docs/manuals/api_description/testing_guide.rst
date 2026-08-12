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

.. _manual_time_testing:

Unit-Testing Time-Dependent Code
================================

Testing application logic that depends on time can be challenging. To solve this, the ``score::time`` framework provides a powerful mechanism to replace the real-time clock with a controllable "fake" clock during unit tests. This is achieved using the ``ScopedClockOverride`` helper.

Using Existing Test Utilities
=============================

The framework provides ``ClockTestFactory<Tag>`` in
``score/time/clock/src/clock_test_factory.h`` for constructor-based mock injection.

Use this helper when your component accepts ``Clock<Tag>`` via constructor or setter injection.

.. code-block:: cpp

   #include "score/time/clock/src/clock_test_factory.h"
   #include "score/time/clock/src/clock_backend_mock.h"
   #include "score/time/vehicle_time.h"
   #include <memory>

   auto backend = std::make_shared<score::time::test_utils::ClockBackendMock<score::time::VehicleTime>>();
   auto clock = score::time::test_utils::ClockTestFactory<score::time::VehicleTime>::Make(backend);

When code under test calls ``Clock<Tag>::GetInstance()`` internally, use
``ScopedClockOverride<Tag>`` as shown below.


Example: Testing a Timeout Handler
==================================

This example demonstrates how to test a component that performs an action once a specific timeout duration has elapsed.

**Component to be tested (`my_component.h`):**

.. code-block:: cpp

   #include "score/time/clock.h"
   #include "score/time/vehicle_time.h"
   #include <chrono>

   class MyTimeoutHandler {
   public:
       MyTimeoutHandler()
           : clock_{score::time::Clock<score::time::VehicleTime>::GetInstance()}
           , start_time_{clock_.Now().TimePoint()} {}

       bool HasTimedOut(std::chrono::seconds timeout_duration) {
           const auto now = clock_.Now().TimePoint();
           return (now - start_time_) > timeout_duration;
       }

   private:
       score::time::Clock<score::time::VehicleTime> clock_;
       score::time::VehicleTime::time_point start_time_;
   };

**Unit Test (`my_component_test.cpp`):**

.. code-block:: cpp

   #include "my_component.h"
   #include "score/time/clock/src/clock_backend_mock.h"
   #include "score/time/clock/src/scoped_clock_override.h"
   #include <gtest/gtest.h>

   TEST(MyTimeoutHandlerTest, DetectsTimeoutCorrectly)
   {
       auto fake_clock_backend =
           std::make_shared<score::time::test_utils::ClockBackendMock<score::time::VehicleTime>>();
       score::time::VehicleTime::duration elapsed{0};

       ON_CALL(*fake_clock_backend, Now())
           .WillByDefault(testing::Invoke([&elapsed]() {
               return score::time::TimeSnapshot<score::time::VehicleTime>{
                   score::time::VehicleTime::time_point{elapsed}};
           }));

       // 1. Activate override because component uses Clock<VehicleTime>::GetInstance().
       auto clock_override = score::time::test_utils::ScopedClockOverride<score::time::VehicleTime>(
           fake_clock_backend);

       // 2. Instantiate component-under-test. It now uses fake backend.
       MyTimeoutHandler handler;
       const auto timeout = std::chrono::seconds{10};

       // 3. Initially, no timeout should be detected.
       EXPECT_FALSE(handler.HasTimedOut(timeout));

       // 4. Advance fake time by 9 seconds.
       elapsed += std::chrono::seconds{9};
       EXPECT_FALSE(handler.HasTimedOut(timeout));

       // 5. Advance past 10-second threshold (total: 11 seconds).
       elapsed += std::chrono::seconds{2};
       EXPECT_TRUE(handler.HasTimedOut(timeout));

   } // clock_override is destroyed here


Bazel BUILD Setup
=================

Because ``ScopedClockOverride`` modifies global state (the active backend for a given clock tag), tests utilizing it must be configured carefully in Bazel.

To prevent parallel tests from overriding the clock simultaneously and interfering with each other, you **must** mark your test targets with the ``exclusive`` tag.

.. code-block:: python

   cc_test(
       name = "my_component_test",
       srcs = [
           "my_component_test.cpp",
           "clock_test_factory.h"
       ],
       tags = ["exclusive", "unit"],  # "exclusive" prevents parallel execution conflicts
       deps = [
           ":my_component",
           "//score/time/vehicle_time:vehicle_time_mock",
           "@googletest//:gtest",
           "@googletest//:gtest_main",
       ],
   )
