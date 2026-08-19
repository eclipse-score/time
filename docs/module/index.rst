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

Module
======

The S-CORE ``time`` module provides a unified API for accessing system, steady, high-resolution steady, and PTP-synchronized vehicle time. The module contains four components: a client library for application-facing access, Time Slave for PTP clock synchronization, ``ts_client`` for shared-memory IPC between Time Slave and Time Daemon, and Time Daemon for synchronization quality validation before serving Vehicle Time.

.. code-block:: rst

   .. mod:: Time
      :id: mod__time
      :includes: comp__component_name_template

Module View
-----------

.. code-block:: rst

   .. mod_view_sta:: Time Module Static View
      :id: mod_view_sta__time__time
      :includes: comp__component_name_template

      .. needarch::
         :scale: 50
         :align: center

         {{ draw_module(need(), needs) }}

Module Documents
----------------

.. toctree::
   :maxdepth: 1
