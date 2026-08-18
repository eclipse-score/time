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

API Usage: Accessing Supported Time Bases
=========================================

The primary interface for applications to access time values is the ``score::time`` client library. It provides a simple, robust, and testable way to get current time from all supported time bases.

This section describes the most common use case: polling current time snapshots.

Supported time bases in this module:

* ``std::chrono::system_clock`` via ``score::time::SystemClock``
* ``std::chrono::steady_clock`` via ``score::time::SteadyClock``
* ``score::time::HighResSteadyTime`` via ``score::time::HighResSteadyClock``
* ``score::time::VehicleTime`` via ``score::time::VehicleClock``

For more detail, see the :ref:`module user manual<user_manual>`.

Polling Local Time Bases
------------------------

All supported clocks use the same API shape: ``GetInstance()`` and ``Now()``.

.. code-block:: cpp

   #include "score/time/system_time/src/system_clock.h"
   #include "score/time/steady_time/src/steady_clock.h"
   #include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

   void poll_local_time_bases()
   {
       // Local time bases can be accessed without any initialization.
       const auto system_snapshot = score::time::SystemClock::GetInstance().Now();
       const auto steady_snapshot = score::time::SteadyClock::GetInstance().Now();
       const auto high_res_snapshot = score::time::HighResSteadyClock::GetInstance().Now();

       // Access the timepoint from every snapshot in the same way.
       const auto system_tp = system_snapshot.TimePoint();
       const auto steady_tp = steady_snapshot.TimePoint();
       const auto high_res_tp = high_res_snapshot.TimePoint();
   }

Polling Vehicle Time with Quality Checks
----------------------------------------

This method involves actively requesting the current vehicle time from the ``score::time`` framework. It is the simplest way to get a timepoint when needed.

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

       // 2. Ensure the clock is initialized and ready to provide time snapshots.
       if (!clock.Init()) {
       std::cerr << "Error: VehicleClock failed to initialize. "
               << "Ensure TimeSlave and TimeDaemon are running." << std::endl;
           return;
       }

       // 3. Request the current time snapshot.
       //    This call retrieves the latest time information from the TimeDaemon via IPC.
       const auto snapshot = clock.Now();

       // 4. Check the status of the snapshot.
       //    IsConsistent(): status flags are not contradictory.
       //    IsReliable(): clock has synchronized at least once in this lifecycle and no active timeout/leap fault.
       //    IsSynchronized() is also available to check if the clock has ever synchronized, but it does not check for timeout/leap faults.
       const auto status = snapshot.Status();
       if (status.IsConsistent() && status.IsReliable())
       {
           // 5. Use the timepoint.
           //    The timepoint is a std::chrono::time_point.
           const auto current_time = snapshot.TimePoint();
           const auto ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
               current_time.time_since_epoch()).count();

           std::cout << "Successfully retrieved reliable Vehicle Time: "
                     << ns_since_epoch << " ns since epoch." << std::endl;
       }
       else
       {
           // 6. Handle invalid or currently unusable status.
           //    Applications must not use TimePoint() if status is inconsistent,
           //    never synchronized, or currently unreliable.
           std::cerr << "Warning: Vehicle Time status is not usable yet. "
                     << "Retrying later..." << std::endl;
       }
   }

.. attention::

    Never use ``TimePoint`` from ``ClockSnapshot`` before verifying status.
    For robust handling, check ``Status().IsConsistent()``, ``Status().HasBeenSynchronized()``, and ``Status().IsReliable()``.

Advanced API Usage: Subscribing to PTP Protocol Events
======================================================

For advanced use cases, such as diagnostics, network monitoring, or detailed performance analysis, the ``score::time`` framework allows applications to subscribe directly to low-level PTP protocol data events. Instead of polling for the final, processed time, an application can register a callback function that is invoked asynchronously whenever new data arrives from the ``TimeSlave``.

.. warning::

   This is an advanced feature. Most applications should use the simpler polling mechanism described in the previous chapter, as it provides the fully quality-assured time. Subscribing to raw PTP data bypasses some of the quality checks performed by the ``TimeDaemon``.

Available Data Subscriptions
----------------------------

Two types of data events can be subscribed to:

1.  **`TimeSlaveSyncData`**:
    This event is triggered whenever the ``TimeSlave`` successfully processes a PTP Sync/Follow-Up message pair from the Time Master. The data contains raw offset and rate correction information, as well as the underlying hardware and software timestamps.

2.  **`PDelayMeasurementData`**:
    This event is triggered after the ``TimeSlave`` completes a peer-delay measurement cycle (PDelay_Req/Resp/FUp exchange). The data contains the calculated path delay to the communication partner.

Subscribing to Events
---------------------

The following code example demonstrates how to register, handle, and unregister callbacks for these events.

.. code-block:: cpp

   #include "score/time/clock.h"
   #include "score/time/vehicle_time.h"
   #include <atomic>
   #include <iostream>
   #include <mutex>

   // A thread-safe data handler for our application
   class PtpDataLogger
   {
   public:
       void HandleSyncData(const score::time::TimeSlaveSyncData<score::time::VehicleTime>& data)
       {
           std::lock_guard<std::mutex> lock(mutex_);
           std::cout << "PTP Sync Event: Offset = " << data.offset_ns
                     << " ns, Rate Ratio = " << data.rate_ratio << std::endl;
           // Further processing of the data...
       }

       void HandlePDelayData(const score::time::PDelayMeasurementData<score::time::VehicleTime>& data)
       {
           std::lock_guard<std::mutex> lock(mutex_);
           std::cout << "PTP PDelay Event: Path Delay = " << data.path_delay_ns << " ns" << std::endl;
           // Further processing of the data...
       }

   private:
       std::mutex mutex_;
   };

   /**
    * @brief Demonstrates how to subscribe to and unsubscribe from PTP protocol events.
    */
   void subscribe_to_ptp_events()
   {
       auto& clock = score::time::Clock<score::time::VehicleTime>::GetInstance();
       PtpDataLogger logger;

       // 1. Subscribe to Sync data events using a lambda that calls our thread-safe handler.
       //    The returned handle is used later to unsubscribe.
       auto sync_subscription = clock.Subscribe<score::time::TimeSlaveSyncData<score::time::VehicleTime>>(
           [&logger](const auto& data) { logger.HandleSyncData(data); });

       std::cout << "Subscribed to TimeSlaveSyncData events." << std::endl;


       // 2. Subscribe to Peer-Delay data events.
       auto pdelay_subscription = clock.Subscribe<score::time::PDelayMeasurementData<score::time::VehicleTime>>(
           [&logger](const auto& data) { logger.HandlePDelayData(data); });

       std::cout << "Subscribed to PDelayMeasurementData events." << std::endl;

       // ... application runs and receives callbacks asynchronously ...
       std::this_thread::sleep_for(std::chrono::seconds(10));


       // 3. Unsubscribe when the data is no longer needed.
       //    The subscription handle is moved into the Unsubscribe call.
       clock.Unsubscribe(std::move(sync_subscription));
       std::cout << "Unsubscribed from TimeSlaveSyncData events." << std::endl;

       clock.Unsubscribe(std::move(pdelay_subscription));
       std::cout << "Unsubscribed from PDelayMeasurementData events." << std::endl;
   }


Threading and Safety Considerations
-----------------------------------

.. attention::

   Callback functions are executed on a **backend thread** owned by the ``score::time`` framework, not on the application's main thread. Therefore, all callback handlers **must be thread-safe**.

*   **Data Protection**: Use mutexes, atomics, or other synchronization primitives to protect any shared data that is accessed or modified within the callback.
*   **Keep it Short**: Callbacks should be lightweight and non-blocking. Offload any time-consuming processing to a separate application-owned thread to avoid delaying the ``score::time`` backend.

Unsubscribing
-------------

It is crucial to unsubscribe from events when they are no longer needed to prevent resource leaks and dangling callbacks. The ``Subscribe`` method returns a handle object which must be passed to the ``Unsubscribe`` method. The handle is invalidated upon unsubscription.
