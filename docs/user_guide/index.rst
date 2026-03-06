..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

.. _inc_time_user_guide:

##########
User Guide
##########

Prerequisites
=============

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Requirement
     - Details
   * - gPTP Time Master
     - An IEEE 802.1AS-compliant master (e.g., a TSN switch or another ECU
       running a gPTP master stack) reachable on the target Ethernet interface.
   * - PTP Hardware Clock
     - A NIC with hardware timestamping support (``/dev/ptp0`` on Linux).
       Software timestamps are NOT supported.
   * - OS privileges
     - ``tsyncd`` requires ``CAP_NET_RAW`` (Linux) or equivalent QNX
       privileges to open a raw Ethernet socket.
   * - POSIX shared memory
     - ``/dev/shm`` must be mounted (Linux default). On QNX the equivalent
       shared memory subsystem must be active.

Running tsyncd
==============

1. Copy the default configuration file and edit as needed:

   .. code-block:: bash

      cp src/tsyncd/config/tsyncd.conf /etc/tsyncd.conf

2. Start the daemon:

   .. code-block:: bash

      # Linux — requires CAP_NET_RAW
      sudo ./tsyncd --config /etc/tsyncd.conf

      # or without sudo if the binary has the capability set:
      sudo setcap cap_net_raw+ep ./tsyncd
      ./tsyncd --config /etc/tsyncd.conf

3. Verify it is running and synchronised:

   .. code-block:: bash

      # The shared memory region should appear immediately:
      ls -la /dev/shm/score_time_vehicle_time

      # Read vehicle time via the test client (if built):
      bazel run //examples:score_time_example

Configuration Reference
=======================

Edit ``tsyncd.conf``:

.. code-block:: ini

   # Network interface for gPTP (must support SO_TIMESTAMPING)
   iface_name = eth0

   # PTP Hardware Clock device
   phc_device = /dev/ptp0

   # Shared memory
   shm_name = /score_time_vehicle_time
   shm_size = 4096

   # NTP servers for absolute time (optional)
   ntp_servers = pool.ntp.org,time.cloudflare.com
   ntp_query_interval_ms = 1000
   ntp_samples_to_lock = 5

   # Sync quality thresholds (nanoseconds)
   sync_timeout_ms = 2000
   pdelay_timeout_ms = 4000
   unstable_offset_threshold_ns = 1000
   jump_future_threshold_ns = 1000000000

   # External absolute time source via Unix socket (optional)
   abs_external_enable = false
   abs_source_socket = /var/run/gnss_time.sock
   abs_source_timeout_ms = 5000

Using libscore_time
===================

Add the library to your Bazel target:

.. code-block:: python

   cc_binary(
       name = "my_app",
       srcs = ["main.cpp"],
       deps = ["//src/libscore_time:score_time"],
   )

Basic Usage
-----------

.. code-block:: cpp

   #include <score_time.hpp>
   #include <iostream>

   int main() {
       auto st = score_time::ScoreTime::Create({});
       if (!st) {
           std::cerr << "Failed to open shared memory — is tsyncd running?\n";
           return 1;
       }

       // Read vehicle time
       auto& vt  = st->VehicleTime();
       auto  acc = vt.GetAccuracyQualifier();

       if (acc == score_time::AccuracyQualifier::kSynchronized) {
           auto t = vt.Now();
           std::cout << "Vehicle time: " << t.count() << " ns\n";
       } else {
           std::cout << "Not synchronized: " << static_cast<int>(acc) << "\n";
       }

       // Read absolute (UTC) time
       auto& at     = st->AbsoluteTime();
       auto  utc    = at.Now();
       auto  inaccNs = at.GetEstimatedInaccuracyNs();
       std::cout << "UTC: " << utc.count() << " ns  (±" << inaccNs << " ns)\n";

       return 0;
   }

Reading Diagnostic Logs
-----------------------

.. code-block:: cpp

   score_time::SyncLogEntry entries[64];
   std::size_t n = st->ReadVehicleSyncLog(entries, 64);

   for (std::size_t i = 0; i < n; ++i) {
       std::cout << "event=" << static_cast<int>(entries[i].event)
                 << " mono_ns=" << entries[i].mono_ns
                 << " v1=" << entries[i].value1 << "\n";
   }

Accuracy Qualifier Handling
----------------------------

Always check ``AccuracyQualifier`` before using the time value in
safety-relevant decisions:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Value
     - Recommended action
   * - ``kNoTime``
     - tsyncd has not started or shared memory is uninitialised. Do not use
       the time value.
   * - ``kNotSynchronized``
     - tsyncd is running but no gPTP master is visible. Reject or flag the
       time value for safety-critical paths.
   * - ``kSynchronized``
     - Time value is valid and locked to gPTP master. Safe to use.
   * - ``kUnstable``
     - Synchronization has degraded (large offset). Use with caution;
       consider reverting to a fallback time source.
   * - ``kTimeJumpDetected``
     - A discontinuous time step occurred. Do not rely on delta calculations
       across this event.

Running the Tests
=================

.. code-block:: bash

   # Run all unit tests
   bazel test //tests/cpp/...

   # Run a specific test with verbose output
   bazel test //tests/cpp:test_shared_state --test_output=all

   # Run tests on QNX (cross-compiled)
   bazel test //tests/cpp/... --platforms=@score_bazel_platforms//platforms:qnx_aarch64

Building the Documentation
==========================

.. code-block:: bash

   # One-shot HTML generation
   bazel run //:docs

   # Live preview with auto-reload (opens http://127.0.0.1:8000)
   bazel run //:live_preview

   # Strict build (warnings-as-errors)
   bazel test //:docs_check
