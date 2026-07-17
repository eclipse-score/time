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

A Helper for Controllable Time: The `ClockTestFactory`
======================================================

To make tests cleaner and more readable, it is a recommended practice to create a small test factory helper class. This class encapsulates the creation of the fake clock and provides a simple API to control the time within a test.

Here is a minimal implementation of such a factory. You can add this helper to your own test utilities.

**`clock_test_factory.h` (Example Implementation):**

.. code-block:: cpp

   #include "score/time/clock/src/clock_backend_mock.h"
   #include "score/time/vehicle_time.h"
   #include <chrono>
   #include <memory>

   // A helper class to manage a fake clock backend in tests.
   class ClockTestFactory {
   public:
       // Creates the backend and returns a shared_ptr to it.
       // This backend is then passed to the ScopedClockOverride.
       std::shared_ptr<score::time::test_utils::ClockBackendMock<score::time::VehicleTime>>
       CreateFakeClock() {
           fake_clock_backend_ = std::make_shared<score::time::test_utils::ClockBackendMock<score::time::VehicleTime>>();
           return fake_clock_backend_;
       }

       // Advances the time on the created fake clock.
       void AdvanceTime(std::chrono::nanoseconds duration) {
           // We simulate a monotonic clock by shifting the offset of the mock
           // to return a progressively advanced timestamp on every subsequent call.
           current_time_ += duration;
           ON_CALL(*fake_clock_backend_, Now())
               .WillByDefault(testing::Return(score::time::TimeSnapshot<score::time::VehicleTime>(
                   score::time::VehicleTime::time_point(current_time_))));
       }

   private:
       std::shared_ptr<score::time::test_utils::ClockBackendMock<score::time::VehicleTime>> fake_clock_backend_;
       std::chrono::nanoseconds current_time_{0};
   };


Example: Testing a Timeout Handler
==================================

This example demonstrates how to use the custom `ClockTestFactory` helper to test a component that performs an action once a specific timeout duration has elapsed.

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
   #include "clock_test_factory.h" // Our custom helper
   #include "score/time/clock/src/scoped_clock_override.h"
   #include <gtest/gtest.h>

   TEST(MyTimeoutHandlerTest, DetectsTimeoutCorrectly)
   {
       // 1. Create our test factory helper.
       ClockTestFactory test_factory;
       auto fake_clock_backend = test_factory.CreateFakeClock();

       // 2. Activate the override with the backend from our factory.
       auto clock_override = score::time::test_utils::ScopedClockOverride<score::time::VehicleTime>(
           fake_clock_backend);

       // 3. Instantiate the component-under-test. It will now automatically use the fake clock.
       MyTimeoutHandler handler;
       const auto timeout = std::chrono::seconds{10};

       // 4. Initially, no timeout should be detected.
       EXPECT_FALSE(handler.HasTimedOut(timeout));

       // 5. Advance the fake clock's time via our factory helper by 9 seconds.
       test_factory.AdvanceTime(std::chrono::seconds{9});
       EXPECT_FALSE(handler.HasTimedOut(timeout));

       // 6. Advance the time past the 10 seconds timeout threshold (Total: 11 seconds).
       test_factory.AdvanceTime(std::chrono::seconds{2});
       EXPECT_TRUE(handler.HasTimedOut(timeout));

   } // <-- 7. Here, `clock_override` is destroyed, and the real clock backend is automatically restored.


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
