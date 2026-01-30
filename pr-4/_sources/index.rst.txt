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

Time Synchronization
====================

This documentation describes the structure, usage and configuration of the Time Synchronization module.

.. toctree::
   :maxdepth: 5

   stakeholder_requirements/index
   tool_requirements/index

Overview
--------

This repository provides a standardized setup for projects using **C++** or **Rust** and **Bazel** as a build system.
It integrates best practices for build, test, CI/CD and documentation.

Project Layout
--------------

The module template includes the following top-level structure:

- `src/`: Software components of the time feature (incl. unit tests)
- `tests/`: Future integration tests
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

Configuration
-------------

The `project_config.bzl` file defines metadata used by Bazel macros.

Example:

.. code-block:: python

   PROJECT_CONFIG = {
       "asil_level": "QM",
       "source_code": ["cpp"]
   }

This enables conditional behavior (e.g., choosing `clang-tidy` for C++ or `clippy` for Rust).
