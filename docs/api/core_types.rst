.. ******************************************************************************
   Copyright (c) 2025 Contributors to the Eclipse Foundation

   See the NOTICE file(s) distributed with this work for additional
   information regarding copyright ownership.

   This program and the accompanying materials are made available under the
   terms of the Apache License Version 2.0 which is available at
   https://www.apache.org/licenses/LICENSE-2.0

   SPDX-License-Identifier: Apache-2.0
   ******************************************************************************

Core Types API Reference
========================

.. contents::
   :local:
   :depth: 2

Overview
--------

This module provides core types for the SCORE Time Synchronization system.


Namespace: ``score_time::ipc``
------------------------------


SyncLogEntry
~~~~~~~~~~~~

.. cpp:struct:: score_time::ipc::SyncLogEntry

   Single entry in the synchronization event log Uses per-entry seqlock protocol for lock-free concurrent access between writer (tsyncd daemon) and readers (client applications). **Seqlock Protocol:** - seq is even (0, 2, 4, ...) when entry is readable - seq is odd (1, 3, 5, ...) when writer is updating the entry - Readers retry if seq is odd or changes during read All fields are atomic to prevent torn reads on architectures where 64-bit loads are not naturally atomic.

   **Defined in:** :code:`shared_state.hpp`


SharedState
~~~~~~~~~~~

.. cpp:struct:: score_time::ipc::SharedState

   Main shared memory structure for IPC between tsyncd daemon and clients This structure is mapped into shared memory (typically /dev/shm/score_time) and provides lock-free access to synchronized time information using seqlock protocol. **Memory Layout:** - Header fields (magic, version, size) for validation - Vehicle time data (gPTP/IEEE 802.1AS synchronized time) - Absolute time data (UTC/wall-clock time from GNSS/NTP) - Circular event logs for diagnostics **Concurrency:** - Single writer (tsyncd daemon) - Multiple readers (client applications) - Uses double-buffering seqlock for vehicle/absolute time - Uses per-entry seqlock for log entries **Version History:** - v1-v3: Original implementations - v4: Added per-entry seqlock to prevent torn reads in logs

   **Defined in:** :code:`shared_state.hpp`

   **Usage:** Shared memory structure for lock-free IPC.


Examples
--------

See the test files for usage examples:

- Unit tests: ``tests/cpp/``
- Integration tests: ``tests/integration/``

