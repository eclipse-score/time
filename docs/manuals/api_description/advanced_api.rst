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

.. _manual_time_advanced_api:

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
