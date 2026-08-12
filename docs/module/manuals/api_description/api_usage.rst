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

For more detail, see the :ref:`time library user manual<_time_component_user_manual>`.

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
       //    IsConsistent(): status flags are not contradictory.
       //    HasBeenSynchronized(): clock has synchronized at least once in this lifecycle.
       //    IsReliable(): synchronized now and no active timeout/leap fault.
       const auto status = snapshot.Status();
       if (status.IsConsistent() && status.HasBeenSynchronized() && status.IsReliable())
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
           // 5. Handle invalid or currently unusable status.
           //    Applications must not use TimePoint() if status is inconsistent,
           //    never synchronized, or currently unreliable.
           std::cerr << "Warning: Vehicle Time status is not usable yet. "
                     << "Retrying later..." << std::endl;
       }
   }

.. attention::

    Never use ``TimePoint`` from ``ClockSnapshot`` before verifying status.
    For robust handling, check ``Status().IsConsistent()``, ``Status().HasBeenSynchronized()``, and ``Status().IsReliable()``.
