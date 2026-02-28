.. ******************************************************************************
   Copyright (c) 2025 Contributors to the Eclipse Foundation

   See the NOTICE file(s) distributed with this work for additional
   information regarding copyright ownership.

   This program and the accompanying materials are made available under the
   terms of the Apache License Version 2.0 which is available at
   https://www.apache.org/licenses/LICENSE-2.0

   SPDX-License-Identifier: Apache-2.0
   ******************************************************************************

IPC API Reference
=================

.. contents::
   :local:
   :depth: 2

Overview
--------

This module provides ipc for the SCORE Time Synchronization system.


Namespace: ``score_time::ipc``
------------------------------


ShmRegion
~~~~~~~~~

.. cpp:class:: score_time::ipc::ShmRegion

   RAII wrapper for POSIX shared memory regions Manages lifecycle of shared memory objects (/dev/shm on Linux, shared memory objects on QNX) with automatic cleanup. Uses shm_open + mmap for portable POSIX shared memory access. **Features:** - RAII: Automatically unmaps and closes on destruction - Move-only: Prevents accidental copies of memory mappings - Create-or-open semantics for both producer and consumer - Size validation for safety **Typical Usage:** - Producer (tsyncd): Open with create_or_open=true, writes SharedState - Consumer (clients): Open with create_or_open=false, reads SharedState

   **Defined in:** :code:`shm_region.hpp`


Examples
--------

See the test files for usage examples:

- Unit tests: ``tests/cpp/``
- Integration tests: ``tests/integration/``

