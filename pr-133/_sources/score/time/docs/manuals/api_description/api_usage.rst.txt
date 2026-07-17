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

.. _manual_time_api_usage:

API Usage: Accessing Vehicle Time
=================================

The primary interface for applications to access synchronized time is the ``score::time`` client library. It provides a simple, robust, and testable way to get the current time without dealing with the underlying complexities of PTP and IPC.

This section describes the most common use case: polling the current Vehicle Time.

Polling the Current Time
------------------------

This method involves actively requesting the current time from the ``score::time`` framework. It is the simplest way to get a timepoint when needed.

.. code-block:: cpp

   #include "score/time/clock.h"
   #include "score/time/vehicle_time.h"
   #include <iostream>
   #include <thread>

   /**
    * @brief Demonstrates how to poll the current Vehicle Time and check its status.
    */
   void poll_vehicle_time()
   {
       // 1. Get a handle to the VehicleClock singleton instance.
       auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();

       // 2. Request the current time snapshot.
       //    This call retrieves the latest time information from the TimeDaemon via IPC.
       const auto snapshot = clock.Now();

       // 3. Check the status of the snapshot.
       //    The IsReliable() flag indicates if the time is currently synchronized
       //    to a master and has passed all quality checks in the TimeDaemon.
       if (snapshot.Status().IsReliable())
       {
           // 4. Use the timepoint.
           //    The timepoint is a std::chrono::time_point.
           const auto current_time = snapshot.TimePoint();
           const auto ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
               current_time.time_since_epoch()).count();

           std::cout << "Successfully retrieved reliable Vehicle Time: "
                     << ns_since_epoch << " ns since epoch." << std::endl;
       }
       else
       {
           // 5. Handle the "not synchronized" case.
           //    If the time is not reliable, applications must not use the timepoint value.
           //    This can happen during startup or if the connection to the Time Master is lost.
           //    The application should implement a retry-logic or fallback.
           std::cerr << "Warning: Vehicle Time is not synchronized or not reliable. "
                     << "Retrying later..." << std::endl;
       }
   }

.. attention::

   Never use the ``TimePoint`` from a ``ClockSnapshot`` without first verifying that ``Status().IsReliable()`` is true. Using an unreliable timepoint can lead to incorrect or inconsistent behavior in safety-critical applications.
