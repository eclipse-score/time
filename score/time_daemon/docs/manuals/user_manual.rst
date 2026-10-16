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

Link-Time Composition
=====================

``TimeDaemon`` runs the handlers returned by ``score::td::CreateTimebaseHandlers()``. The stock
``//score/time_daemon:time_daemon`` executable links a default provider that returns one SVT handler. A vendor
executable links its own provider instead, to run different or additional handlers with the same process entry point.

The following public targets are available under ``@score_time//score/time_daemon``:

* ``time_daemon_main``: process entry point, without provider and logging backend.
* ``time_daemon_core``: ``TimeDaemon`` without the process entry point. Its constructor also accepts handlers directly.
* ``timebase_handler``: handler interface.
* ``timebase_handler_factory``: declaration of ``CreateTimebaseHandlers()``.
* ``svt_handler``: ``CreateSvtTimebase()`` to reuse the SVT handler, without the stock provider.

A provider defines ``CreateTimebaseHandlers()`` in one source file and returns all handlers as a
``std::vector<std::unique_ptr<TimebaseHandler>>``. Add them with ``push_back`` or ``emplace_back``, because an
initializer list cannot copy the move-only pointers.

Each executable must link exactly one provider. Set ``alwayslink = True`` on every provider library, because a static
library would otherwise hide a duplicate provider: the linker only extracts the first one. Weak symbols and link order
are not supported as an override mechanism.

For example, a vendor executable that combines its own handler with the SVT handler:

.. code-block:: python

    cc_library(
        name = "vendor_timebases",
        srcs = ["vendor_timebases.cpp"],
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

The vendor workspace provides its own toolchain and build configuration, because the ``.bazelrc`` of this module is
not inherited. It also selects the logging backend.

If no provider is linked, the linker reports an undefined reference to ``score::td::CreateTimebaseHandlers()``. If more
than one is linked, it reports multiple definitions of it.

Handler Lifecycle
=================

``TimeDaemon`` owns the handlers. ``Initialize()`` fails, without initializing any handler, if the composition is empty
or contains a null entry. Otherwise it initializes the handlers in vector order.

After a successful ``Initialize()``, ``Run()`` calls ``RunOnce()`` of every handler in vector order, waits 100 ms and
repeats this until a stop is requested. A stop requested during a pass does not skip the remaining handlers of that
pass. Then ``Stop()`` is called once per handler in reverse order, also if the stop was requested before the first
pass. ``RunOnce()`` must not block, so that one handler cannot delay the others.

Handlers do not arbitrate shared outputs, so the provider has to assign distinct resources. For example, two SVT
handlers publish to the same ``/svt_shmem_path``.

Handlers Built from Machines
============================

A handler built like ``SvtHandler`` derives from ``GenericTimebaseHandler<T>`` and implements ``CreateComponents()``.
There it adds its machines with ``Add()`` and declares their wiring with ``Publish()`` and ``Subscribe()``. The
ProactiveMachines start in the order of the ``Add()`` calls and stop in reverse order, so add the machines that must
run first before the machines that publish data.

These public targets provide the units that ``SvtHandler`` is built from:

* ``generic_timebase_handler``: ``GenericTimebaseHandler<T>``.
* ``msg_broker``: ``Topic``, to declare the wiring.
* ``ptp_time_info``: ``PtpTimeInfo``, the message type of ``SvtHandler``.
* ``ptp_control_flow_divider``: ``CreatePtpControlFlowDivider()``.
* ``gptp_shm_machine``: ``CreateGPTPShmMachine()``, the PTP machine that reads the data published by ``TimeSlave``.
* ``svt_verification_machine``: ``CreateSvtVerificationMachine()``.
* ``svt_publisher``: ``CreateSvtPublisher()``, the publisher that writes ``/svt_shmem_path``.

The factories fix the settings of the machines, such as the 50 ms update interval of the PTP machine and the thresholds
of the verification machine.

Runtime Requirements
====================

The ``TimeDaemon`` requires:

* ``TimeSlave`` must be running and publishing data to shared memory
* Access to POSIX shared memory segment (``/gptp_ptp_info``)
* Managed by system service manager (e.g., `systemd` on Linux, launch script on QNX)
