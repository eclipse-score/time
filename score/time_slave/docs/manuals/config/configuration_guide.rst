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

.. _time_slave_configuration:

TimeSlave Configuration
=======================

The behavior of the ``TimeSlave`` is controlled by the ``GptpEngineOptions`` structure. Currently, only a subset of these options can be overridden at runtime via command-line arguments. For all other options, the hard-coded default values are used.

Command-Line Arguments
-----------------------

The following argument is available to configure the ``TimeSlave`` at runtime: <tbd>


Default Configuration (`GptpEngineOptions`)
--------------------------------------------

The following table lists all available options and their default values as defined in the source code. Currently, only ``iface_name`` can be changed without recompiling the application.

.. list-table:: GptpEngineOptions Default Values
   :widths: 25 15 60
   :header-rows: 1

   * - Option
     - Default Value
     - Description
   * - ``iface_name``
     - ``"emac0"``
     - The network interface to use for gPTP traffic.
   * - ``pdelay_interval_ms``
     - ``1000``
     - The interval in milliseconds for sending Peer-Delay measurement requests.
   * - ``pdelay_warmup_ms``
     - ``2000``
     - The initial delay in milliseconds before the first Peer-Delay request is sent.
   * - ``sync_timeout_ms``
     - ``3300``
     - The time in milliseconds without receiving a PTP Sync message before a timeout is declared and the clock is considered unreliable.
   * - ``jump_future_threshold_ns``
     - ``500'000'000``
     - The threshold in nanoseconds (500 ms) for detecting a significant forward time jump.
   * - ``domain_number``
     - ``0``
     - The gPTP domain number. The TimeSlave will only interact with a PTP master in the same domain.
   * - ``phc_config``
     - ``disabled``
     - Configuration for hardware clock (PHC) adjustments. Disabled by default.


Example Invocation
------------------

.. code-block:: bash

   # Start the TimeSlave, overriding the default interface name "emac0"
   ./time_slave

.. attention::
   The runtime configuration is currently incomplete. To change parameters, you must modify the default values in the ``GptpEngineOptions`` structure and recompile the application. A comprehensive configuration mechanism (e.g., via a JSON file) will come soon.
