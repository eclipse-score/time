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
   :belongs_to: comp__time_daemon

Overview
========

The ``TimeDaemon`` component is a system daemon responsible for quality assurance and providing synchronized time to local applications on the ECU. It reads raw synchronization data from shared memory (published by ``TimeSlave``), performs quality checks and plausibility assessments, and provides the final ``score::time`` API to client applications.

For module-level integration and deployment information, see the main module manual.

Configuration
=============

.. toctree::
   :maxdepth: 2

   config/configuration_guide

Composing Timebase Handlers
===========================

The standard way to implement a custom timebase handler is to derive from ``score::td::GenericTimebaseHandler<T>``.
``T`` is the message type exchanged by the handler's machines. Pass a handler name to the base constructor and
override ``void CreateComponents() noexcept`` to create and connect the machines. The generic base manages the
message broker, initialization, startup, and shutdown; custom handlers do not need to reimplement that lifecycle.

In ``CreateComponents()``, call ``Add()`` for each machine and pass its initialization timeout.
Each timeout counts from the start of initialization. If a machine's ``Init()`` has not succeeded by then,
the handler fails and does not start its machines.
Connect the machines with ``Publish()`` and ``Subscribe()``.
Proactive machines start in ``Add()`` order and stop in reverse order.
Add machines that must run first before their data sources.
Do not add or connect machines after ``Initialize()``.

``ScoreVehicleTimebaseHandler`` is the reference implementation of this pattern. Its constructor names the handler,
and ``CreateComponents()`` selects the machines, initialization timeouts, topics, and wiring for Vehicle Time.

The handlers are composed at link time, not selected at runtime. ``TimeDaemon`` runs every handler returned by
``score::td::CreateTimebaseHandlers()``, which the linked factory defines. The default
``//score/time_daemon:time_daemon`` program uses the S-CORE factory, which creates one ``ScoreVehicleTimebaseHandler``.
To use custom handlers, link your own factory into a program using ``time_daemon_main``.

Handler-related code is grouped under ``score/time_daemon/src/application/timebase_handlers``:

.. code-block:: text

    timebase_handlers/
        timebase_handler.h
        timebase_handler_factory.h
        generic/
            generic_timebase_handler.h
        score/
            score_timebase_handler_factory.cpp
            vehicle_time/
                score_vehicle_timebase_handler.h
                score_vehicle_timebase_handler.cpp
                factory.h
                factory.cpp

The contracts and ``generic/`` implementation do not depend on S-CORE's selected handler composition.
The ``score/`` subtree contains the supplied project-specific composition; its Vehicle Time handler selects the machines,
timings, topics, and wiring. Other projects can provide their own compositions without changing the generic code.

The public targets for application and custom handler integration are available under ``@score_time//score/time_daemon``:

Application and entry points:

* ``time_daemon``: default executable with the S-CORE factory and Vehicle Time handler.
* ``time_daemon_core``: reusable daemon library, without the process entry point or a handler factory implementation.
* ``time_daemon_main``: process entry point, without a handler factory or logging backend.

Programs linking ``time_daemon_core`` must provide a factory definition even when injecting handlers directly,
because both constructors are defined in the same source file. Unit tests can supply a test-only factory.

Generic handler infrastructure:

* ``timebase_handler_factory``: declaration of ``CreateTimebaseHandlers()``.
* ``generic_timebase_handler``: standard implementation base, ``GenericTimebaseHandler<T>``.

Define ``CreateTimebaseHandlers()`` in one source file. Return the handlers in a
``std::vector<std::unique_ptr<TimebaseHandler>>``. Add them with ``push_back`` or ``emplace_back``.
Do not use an initializer list: it would copy the pointers, but ``unique_ptr`` cannot be copied.
Include ``score/time_daemon/src/application/timebase_handlers/timebase_handler_factory.h`` in your factory source.
``TimebaseHandler`` remains the polymorphic type shared by the daemon and factory; its declaration is included
by the factory header. Custom handlers inherit that contract through ``GenericTimebaseHandler<T>``.

Each program using ``time_daemon_main`` must supply exactly one factory definition. If it is packaged in a
library, set ``alwayslink = True`` so duplicate providers cause a link error rather than being hidden in archives.
Do not use weak symbols or link order to replace the factory. The :ref:`time_daemon_factory_composition`
section explains provider linkage and ownership.

This example uses a custom generic handler. Add dependencies on the public targets
for any machine factories, message types, and topic headers used by ``vendor_handler``:

.. code-block:: python

    cc_library(
        name = "vendor_handler",
        srcs = ["vendor_handler.cpp"],
        hdrs = ["vendor_handler.h"],
        deps = [
            "@score_time//score/time_daemon:generic_timebase_handler",
        ],
    )

    cc_library(
        name = "vendor_timebases",
        srcs = ["timebase_handler_factory.cpp"],
        deps = [
            ":vendor_handler",
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
With more than one directly linked or force-included factory, it reports multiple definitions of that function.

Handler Lifecycle
=================

``TimeDaemon`` owns the handlers. Call ``Initialize()`` once; it rejects empty lists and null handlers before
initializing any handler. Otherwise, it performs setup in list order. For generic handlers, setup creates and
connects machines but does not yet initialize or start them.

``Run()`` calls ``RunOnce()`` on each handler in list order, waiting up to 100 ms between passes. Generic handlers
initialize their machines asynchronously and start proactive machines only after all initialization succeeds.
A failed initialization calls ``Stop()`` and enters terminal ``kStopped`` without starting machines. A stop request
does not skip the rest of the current pass. Generic ``RunOnce()`` does not check cancellation itself, so a
successful initialization result can still start machines during that pass. Custom ``RunOnce()`` implementations
must not block, or they will delay the other handlers.

On cancellation, ``Run()`` stops handlers in reverse order, even if cancellation preceded the first pass.
When managing a generic handler directly, serialize its lifecycle calls and cancel the parent stop source
before stopping or destroying it with pending initialization. Every ``Stop()`` calls all registered proactive
machines in reverse registration order, even before startup or after an earlier stop, then enters ``kStopped``.
Machines must tolerate these repeated calls, including the additional call during handler destruction. Polling
a stopped handler does nothing; reinitialization and restart are not supported.

See :ref:`time_daemon_handler_lifecycle` for the state diagram, cancellation behavior, and shutdown lifetime details.

Reusable Machine Building Blocks
================================

These optional public targets under ``@score_time//score/time_daemon`` provide the reusable machine building
blocks used by ``ScoreVehicleTimebaseHandler``. Depend on only the ones used by your custom handler:

* ``msg_broker``: ``Topic``, to connect machines.
* ``ptp_time_info``: ``PtpTimeInfo``, the message type of ``ScoreVehicleTimebaseHandler``.
* ``ptp_control_flow_divider``: ``CreatePtpControlFlowDivider()``.
* ``ptp_machine``: ``PTPMachine<PTPEngine>``, the PTP machine for a custom engine.
* ``gptp_shm_machine``: ``CreateGPTPShmMachine()``, the PTP machine that reads ``TimeSlave`` data.

Vehicle Time building blocks (existing SVT API names):

* ``svt_verification_machine``: ``CreateSvtVerificationMachine()``.
* ``svt_publisher``: ``CreateSvtPublisher()``, which writes to ``/svt_shmem_path``.

To read PTP data from another source, write an engine and create its ``PTPMachine`` directly.
Pass the machine's name, update interval, and engine arguments to the constructor.
Then add the machine with its initialization timeout:

.. code-block:: cpp

    auto ptp_machine = std::make_shared<PTPMachine<VendorPtpEngine>>(
        "ptp_worker", std::chrono::milliseconds{50}, vendor_client);
    Add(ptp_machine, std::chrono::seconds{20});

The engine must provide ``bool Initialize()``, ``bool Deinitialize()``, and ``bool ReadPTPSnapshot(PtpTimeInfo&)``.

Runtime Requirements
====================

The supplied S-CORE composition requires:

* ``TimeSlave`` must be running and publishing data to shared memory
* Access to POSIX shared memory segment (``/gptp_ptp_info``)
* Managed by system service manager (e.g., `systemd` on Linux, launch script on QNX)
