..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

TimeDaemon Documentation
========================

This documentation describes the **TimeDaemon** and the **score::time** module.

.. contents:: Table of Contents
   :depth: 2
   :local:

Overview
--------

**TimeDaemon** is a non-AUTOSAR adaptive process designed to provide synchronized vehicle time to client applications.
It supports multiple time bases including **in-vehicle synchronized time** (PTP - Precision Time Protocol) and
**external synchronized time** (absolute time base). The daemon retrieves time information from the respective time sources,
verifies and validates the timepoints, and distributes this time information across multiple clients through efficient IPC mechanisms.

The main responsibilities of TimeDaemon include:

- **Providing current Vehicle time** to different applications
- **Setting synchronization qualifiers** (e.g., Synchronized, Timeout, etc.)
- **Providing diagnostic information** for system monitoring
- **Supporting additional verification mechanisms** such as QualifiedVehicleTime (QVT) for safety-critical applications

For a detailed concept and architectural design, please refer to the :doc:`TimeDaemon Concept Documentation <TimeDaemon/index>`.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   TimeDaemon/index
   time/index

Project Layout
--------------

The module template includes the following top-level structure:

- `src/`: Main C++/Rust sources
- `tests/`: Unit and integration tests
- `examples/`: Usage examples
- `docs/`: Documentation using `docs-as-code`
- `.github/workflows/`: CI/CD pipelines

Quick Start
-----------

To build the module:

.. code-block:: bash

   bazel build //src/...

To run tests:

.. code-block:: bash

   bazel test //tests/...

To build the documentation:

.. code-block:: bash

   bazel run //:docs

Configuration
-------------

The `project_config.bzl` file defines metadata used by Bazel macros.

Example:

.. code-block:: python

   PROJECT_CONFIG = {
       "asil_level": "QM",
       "source_code": ["cpp", "rust"]
   }

This enables conditional behavior (e.g., choosing `clang-tidy` for C++ or `clippy` for Rust).
