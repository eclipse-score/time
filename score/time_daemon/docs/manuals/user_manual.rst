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

.. _time_daemon_user_manual:

Time Daemon User Manual
#######################

.. document:: User Manual Time Daemon Component
   :id: doc__user_manual_time_daemon
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__training_path[version==1]

Overview
========

The ``TimeDaemon`` component is a system daemon responsible for quality assurance and providing synchronized time to local applications on the ECU. It reads raw synchronization data from shared memory (published by ``TimeSlave``), performs quality checks and plausibility assessments, and provides the final ``score::time`` API to client applications.

For module-level integration and deployment information, see the main module manual.

Configuration
=============

.. toctree::
   :maxdepth: 2

   config/configuration_guide

Choosing Handlers
=================

``TimeDaemon`` runs the handlers from ``score::td::CreateTimebaseHandlers()``. The default
``//score/time_daemon:time_daemon`` program uses a factory that creates one SVT handler.
To use other handlers, link your own factory into a program with the same entry point.

These public targets are available under ``@score_time//score/time_daemon``:

* ``time_daemon_main``: process entry point, without a handler factory or logging backend.
* ``time_daemon_core``: ``TimeDaemon`` without the entry point. You can also pass handlers to its constructor.
* ``timebase_handler``: handler interface.
* ``timebase_handler_factory``: declaration of ``CreateTimebaseHandlers()``.
* ``svt_handler``: ``CreateSvtTimebase()`` to reuse the SVT handler without the default factory.

Define ``CreateTimebaseHandlers()`` in one source file. Return the handlers in a
``std::vector<std::unique_ptr<TimebaseHandler>>``. Add them with ``push_back`` or ``emplace_back``.
Do not use an initializer list: it would copy the pointers, but ``unique_ptr`` cannot be copied.

Each program must link exactly one factory library. Set ``alwayslink = True`` on each factory library so duplicate
definitions cause a link error. Without it, the linker may silently use the first library.
Do not use weak symbols or link order to replace the factory.

This example combines a custom handler with the SVT handler:

.. code-block:: python

    cc_library(
        name = "vendor_timebases",
        srcs = ["timebase_handler_factory.cpp"],
        deps = [
            ":vendor_handler",
            "@score_time//score/time_daemon:svt_handler",
            "@score_time//score/time_daemon:timebase_handler_factory",
        ],
        alwayslink = True,
    )

    cc_binary(
        name = "vendor_time_daemon",
        deps = [
            ":vendor_timebases",
            "@score_baselibs//score/mw/log:console_only_backend",
            "@score_time//score/time_daemon:time_daemon_main",
        ],
    )

Without a factory, the linker reports an undefined reference to ``score::td::CreateTimebaseHandlers()``.
With more than one factory, it reports multiple definitions of that function.

Handler Lifecycle
=================

``TimeDaemon`` owns the handlers. ``Initialize()`` fails if the list is empty or any handler is null.
No handlers are initialized on failure. Otherwise, it initializes the handlers in list order.

After ``Initialize()`` succeeds, ``Run()`` calls ``RunOnce()`` on each handler in list order.
It then waits 100 ms and repeats until a stop is requested. A stop request does not skip the rest of the current pass.

``Run()`` then calls ``Stop()`` once on each handler in reverse order.
It also does this if a stop was requested before the first pass.
``RunOnce()`` must not block or it will delay the other handlers.

Handlers Built from Machines
============================

To build a handler like ``SvtHandler``, derive from ``GenericTimebaseHandler<T>`` and implement ``CreateComponents()``.
In ``CreateComponents()``, call ``Add()`` for each machine. Connect the machines with ``Publish()`` and ``Subscribe()``.
Proactive machines start in ``Add()`` order and stop in reverse order.
Add machines that must run first before their data sources.

These public targets provide the parts used by ``SvtHandler``:

* ``generic_timebase_handler``: ``GenericTimebaseHandler<T>``.
* ``msg_broker``: ``Topic``, to connect machines.
* ``ptp_time_info``: ``PtpTimeInfo``, the message type of ``SvtHandler``.
* ``ptp_control_flow_divider``: ``CreatePtpControlFlowDivider()``.
* ``ptp_machine``: ``PTPMachine<PTPEngine>``, the PTP machine for a custom engine.
* ``gptp_shm_machine``: ``CreateGPTPShmMachine()``, the PTP machine that reads ``TimeSlave`` data.
* ``svt_verification_machine``: ``CreateSvtVerificationMachine()``.
* ``svt_publisher``: ``CreateSvtPublisher()``, which writes to ``/svt_shmem_path``.

To read PTP data from another source, write an engine and create its ``PTPMachine`` directly.
Pass the machine's name, update interval, and engine arguments to the constructor:

.. code-block:: cpp

    auto ptp_machine = Add(std::make_shared<PTPMachine<VendorPtpEngine>>(
        "ptp_worker", std::chrono::milliseconds{50}, vendor_client));

The engine must provide ``bool Initialize()``, ``bool Deinitialize()``, and ``bool ReadPTPSnapshot(PtpTimeInfo&)``.

Runtime Requirements
====================

The ``TimeDaemon`` requires:

* ``TimeSlave`` must be running and publishing data to shared memory
* Access to POSIX shared memory segment (``/gptp_ptp_info``)
* Managed by system service manager (e.g., `systemd` on Linux, launch script on QNX)
