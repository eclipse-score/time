.. ******************************************************************************
   Copyright (c) 2025 Contributors to the Eclipse Foundation

   See the NOTICE file(s) distributed with this work for additional
   information regarding copyright ownership.

   This program and the accompanying materials are made available under the
   terms of the Apache License Version 2.0 which is available at
   https://www.apache.org/licenses/LICENSE-2.0

   SPDX-License-Identifier: Apache-2.0
   ******************************************************************************

Utilities API Reference
=======================

.. contents::
   :local:
   :depth: 2

Overview
--------

This module provides utilities for the SCORE Time Synchronization system.


Namespace: ``score_time::utils``
--------------------------------


PthreadLockGuard
~~~~~~~~~~~~~~~~

.. cpp:class:: score_time::utils::PthreadLockGuard

   RAII wrapper for pthread_mutex_t Automatically locks the mutex on construction and unlocks on destruction. This prevents deadlocks from forgotten unlocks or early returns.

   **Defined in:** :code:`pthread_lock_guard.hpp`

   **Usage:** RAII wrapper for automatic resource management.


Examples
--------

See the test files for usage examples:

- Unit tests: ``tests/cpp/``
- Integration tests: ``tests/integration/``

