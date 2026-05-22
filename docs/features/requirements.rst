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

Feature Requirements
====================

This page lists the functional requirements for the ``score::time`` module.


FR-1 — Vehicle Time Synchronization
-----------------------------------


The score::time feature shall synchronize the local clock with an external Time Master using the gPTP protocol (IEEE 802.1AS).


FR-2 — Vehicle Time Sync Precision
----------------------------------


The score::time feature shall synchronize the local time base with the Time Master within a defined precision, based on the system setup.

- For AVB nodes, the maximum difference between two AVB ports/ECUs in SYNC may be 1 μs.
- For non-AVB nodes, the precision may be relaxed to 200–500 μs.


FR-3 — Vehicle Time Base API
----------------------------


The score::time feature shall provide an API to access the synchronized vehicle time.


FR-4 — Vehicle Time Base Accuracy Qualifier
-------------------------------------------


The score::time feature shall provide an API to read the accuracy qualifier of the local synchronized time base.

The qualifier reflects whether:

- the time base is synchronized to an external time source
- time jumps exist (past or future)
- time increases monotonically


FR-5 — Vehicle Time Base Time Point Qualifier
---------------------------------------------


The score::time feature shall provide an API to read the time point qualifier of the local synchronized time base.

The qualifier reflects whether the time point is ASIL-B or QM data.


FR-6 — Vehicle Time Control Flow
--------------------------------


Access to time and metadata shall be fast and efficient, avoiding kernel calls and heavy resource manager involvement.

Use case: frequent access by multiple clients.


FR-7 — Vehicle Time Synchronization Logging
-------------------------------------------


The score::time feature shall provide a mechanism to log the internal state of the synchronization process.

Use case: debugging and diagnostics.


FR-8 — score::time External Synchronization
-------------------------------------------


The score::time feature shall support synchronization with external time sources such as UTC from GPS.


FR-9 — Absolute Time Base API
-----------------------------


The score::time feature shall provide an API to read the absolute time base synchronized to external sources.


FR-10 — Absolute Time Base Accuracy Qualifier
---------------------------------------------


The score::time feature shall provide an API to read the accuracy qualifier of the absolute time base, synchronized to external time sources.


FR-11 — Absolute Time Base Security Qualifier
---------------------------------------------


The score::time feature shall provide an API to read the security qualifier of the absolute time base, synchronized to external time sources.

The security level may be indicated as:

- No time available
- Time not trustworthy
- Time trustworthy
- Time very trustworthy


FR-12 — Absolute Time Synchronization Status Log
------------------------------------------------


The score::time feature shall provide a mechanism to log the internal state of the absolute time synchronization process, to support debugging and diagnosis.


FR-13 — High Precision Clock API
--------------------------------


The score::time feature shall provide an API to read the high precision clock with nanosecond precision.

The clock to which the high precision clock is mapped depends on the system design.

Use case: time-critical applications such as audio/video streaming, event logging, and diagnostics.


FR-14 — Monotonic Clock API
---------------------------


The score::time feature shall provide an API to read a monotonic, non-adjustable clock value.


FR-15 — score::time Mocking APIs
--------------------------------


The score::time feature shall provide support for mocking its public interfaces, enabling unit, component, and integration testing of applications.

