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

Informal Overview
-----------------

This section give an informal overview of the current status of the feature requrements related to
the Time Synchronization functionality.

Time synchronization
********************

+-------------------------------------------+-----------------------------------------------------+
| Feature Requirement ID                    | Informal status                                     |
+===========================================+=====================================================+
| feat_req__time__vehicle_time_sync         | Implemented via modified ptpd                       |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_sync_prec    | Syncing of local clock possible (via ptpd)          |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_time_api     | API present - corresponds to AUTOSAR API R24-11     |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_acc_qual_api | Sync status and leap jump info available            |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_time_pt_qual | -                                                   |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_ctrl_flow    | Time read requires getting local clock              |
|                                           | tsync data from ShMem                               |
+-------------------------------------------+-----------------------------------------------------+
| feat_req__time__vehicle_time_sync_log     | Logging via stdout and stderr                       |
+-------------------------------------------+-----------------------------------------------------+

Time Synchronization to absolute external sources
*************************************************

+-------------------------------------+-----------------------------------------------------------+
| Feature Requirement ID              | Informal status                                           |
+=====================================+===========================================================+
| feat_req__time__abs_sync            | Via ptpd                                                  |
+-------------------------------------+-----------------------------------------------------------+
| feat_req__time__abs_base_api        | Time of a time base can be read via                       |
|                                     | `SynchronizedTimeBaseConsumer`                            |
+-------------------------------------+-----------------------------------------------------------+
| feat_req__time__abs_acc_qual        | Not fully clear - accuracy of time master?                |
+-------------------------------------+-----------------------------------------------------------+
| feat_req__time__abs_sec_qual        | -                                                         |
+-------------------------------------+-----------------------------------------------------------+
| feat_req__time__abs_sync_log        | Logging via stdout and stderr                             |
+-------------------------------------+-----------------------------------------------------------+

Local Clock
***********

+-------------------------------------+-----------------------------------------------------------+
| Feature Requirement ID              | Informal status                                           |
+=====================================+===========================================================+
| feat_req__time__high_prec_clock_api | No special score interface defined,                       |
|                                     | use C++ `std::chrono::high_resolution_clock`              |
+-------------------------------------+-----------------------------------------------------------+
| feat_req__time__monotonic_clock_api | No special score interface defined,                       |
|                                     | use C++ `std::chrono::steady_clock`                       |
+-------------------------------------+-----------------------------------------------------------+

Testability
***********

+-------------------------------------+-----------------------------------------------------------+
| Feature Requirement ID              | Informal status                                           |
+=====================================+===========================================================+
| feat_req__time__mocking_apis        | Mocking possible and available, unit tests available      |
+-------------------------------------+-----------------------------------------------------------+
