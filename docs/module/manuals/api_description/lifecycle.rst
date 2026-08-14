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

.. _manual_time_lifecycle:

Clock Lifecycle Management
==========================

Before an application can read reliable time from clocks like ``VehicleClock``, the underlying backend service must be initialized and ready. The ``Clock`` API provides several functions to manage this lifecycle gracefully.

.. attention::
   These lifecycle functions are primarily relevant for clocks that depend on external services, like ``VehicleClock``. Simpler clocks such as ``SystemClock`` or ``SteadyClock`` are always available and do not require these steps.

Initializing the Clock
----------------------

The ``Init()`` method must be called once to establish the connection to the backend service (e.g., the ``TimeDaemon``). Until ``Init()`` succeeds, any call to ``Now()`` will return a snapshot with a "not ready" or "unknown" status.

.. code-block:: cpp

   #include "score/time/clock.h"
   #include "score/time/vehicle_time.h"
   #include <iostream>

   void initialize_clock()
   {
       auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();

       // Attempt to initialize the connection to the backend.
       // This can be retried if it fails (e.g., if the TimeDaemon is not yet running).
       if (clock.Init())
       {
           std::cout << "Clock backend initialized successfully." << std::endl;
       }
       else
       {
           std::cerr << "Clock backend initialization failed. Please retry." << std::endl;
       }
   }

Waiting for Availability
------------------------

After initialization, the clock might still not be "reliable" because the ``TimeDaemon`` itself is waiting for synchronization with the PTP master. Instead of polling in a loop, applications can use ``WaitUntilAvailable()`` to block efficiently until the clock is ready.

This is the recommended approach for applications that cannot proceed without a valid time source at startup.

.. code-block:: cpp

   #include "score/time/clock.h"
   #include "score/time/vehicle_time.h"
   #include <score/stop_token.hpp>
   #include <iostream>
   #include <chrono>

   void wait_for_reliable_time(const score::cpp::stop_token& stop_token)
   {
       auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();

       if (!clock.Init()) {
           std::cerr << "Initialization failed. Cannot wait for time." << std::endl;
           return;
       }

       // Wait for a maximum of 30 seconds for the clock to become available.
       // The wait will be interrupted if the application's stop_token is triggered.
       const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

       std::cout << "Waiting for VehicleTime to become available..." << std::endl;

       if (clock.WaitUntilAvailable(stop_token, deadline))
       {
           std::cout << "VehicleTime is now available and synchronized!" << std::endl;

           // Now it is safe to start polling or using the time.
           const auto snapshot = clock.Now();
           if (snapshot.Status().IsReliable()) {
               // ... proceed with application logic ...
           }
       }
       else
       {
           std::cerr << "Timed out waiting for VehicleTime. Is the TimeSlave running and synchronized?" << std::endl;
       }
   }


Checking Availability (Non-Blocking)
------------------------------------

For applications that need to perform other tasks while waiting for time, the non-blocking ``IsAvailable()`` method can be used to periodically check the status.

.. code-block:: cpp

   // Inside an application's main loop
   auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();

   if (clock.IsAvailable())
   {
       // Time is ready, perform time-sensitive tasks.
   }
   else
   {
       // Time is not yet ready, perform other tasks.
   }
