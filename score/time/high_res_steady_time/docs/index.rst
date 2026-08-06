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

.. _high_res_steady_time:

HighResSteadyTime
#################

.. note:: Always-ready high-resolution monotonic clock component

.. document:: HighResSteadyTime
   :id: doc__high_res_steady_time
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__cmpt_request
   :tags: high_res_steady_time

Abstract
========

This component provides the ``HighResSteadyClock`` facade. ``Now``
returns a monotonic ``ClockSnapshot`` without prior initialization; the
lifecycle operations (``Init`` / ``IsAvailable`` /
``WaitUntilAvailable``) are compile-time unavailable on this facade.

Specification
=============

* :need:`comp_req__high_res_steady_time__snapshot`

Footnotes
=========

Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   requirements/index
   architecture/index
