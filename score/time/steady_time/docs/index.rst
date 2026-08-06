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

.. _steady_time:

SteadyTime
##########

.. note:: Always-ready monotonic clock component backed by ``std::chrono::steady_clock``

.. document:: SteadyTime
   :id: doc__steady_time
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__cmpt_request
   :tags: steady_time

Abstract
========

This component provides the ``SteadyClock`` facade over
``std::chrono::steady_clock``. It is always ready and does not expose
``Init`` / ``IsAvailable`` / ``WaitUntilAvailable`` — using them on this
facade is a compile-time error.

Specification
=============

* :need:`comp_req__steady_time__snapshot`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   requirements/index
   architecture/index
