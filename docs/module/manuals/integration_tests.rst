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

.. _manual_time_integration_tests:

******************************
End-to-End Integration Tests
******************************

The tests in ``score/tests`` run the real ``time_slave`` and ``time_daemon`` binaries together, in a virtual
machine (QEMU). A software gPTP grandmaster on the host feeds them time. The tests read the resulting vehicle time
and notifications through the public ``score::time`` client API. This checks the feature requirements on the whole time stack, not on single classes as the unit tests do.

The same tests run on two targets:

.. list-table::
   :header-rows: 1
   :widths: 15 40 20 25

   * - Target
     - Image
     - Bazel target suffix
     - Acceleration
   * - Linux x86_64
     - ITF Ubuntu image + ext4 disk on ``/opt``
     - ``_linux_x86_64``
     - KVM (``/dev/kvm``)
   * - QNX aarch64
     - QNX 8.0 IFS with the binaries in ``/opt``
     - ``_qnx_aarch64``
     - none (emulated, slow)

.. contents:: On this page
   :local:
   :depth: 2

Layout
======

.. code-block:: text

   score/tests/
   ├── common/                      shared by both test suites
   │   ├── BUILD                    support library, probe binary, target images
   │   ├── qemu_tests.bzl           qemu_itf_tests() macro: one py_itf_test per target
   │   ├── fixtures.py              pytest fixtures (time stack, probe, grandmaster, ...)
   │   ├── helpers.py               constants, process control, waiting and assertion helpers
   │   ├── gptp_grandmaster.py      minimal IEEE 802.1AS grandmaster with fault injection
   │   ├── known_issues.py          every xfail marker, one per known defect
   │   ├── vehicle_time_probe/      C++ client that prints vehicle time as JSON
   │   ├── config/                  logging.json and time_slave_config.json deployed to /opt/etc
   │   └── qnx_aarch64/             QNX image build file and QEMU configuration
   └── feature/                     tests of the feature requirements (feat_req__time__*)

Feature Tests (``feature/``)
----------------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Module
     - Covers
   * - ``test_clocks.py``
     - SteadyClock and HighResSteadyClock
   * - ``test_startup.py``
     - start-up behaviour and latency
   * - ``test_synchronization.py``
     - synchronization with the grandmaster, loss and recovery

Each test names the requirement IDs it covers with the ``@add_test_properties`` decorator of the ``attribute_plugin``,
so that docs-as-code links the tests to the requirements (see `test to requirement links <https://eclipse-score.github.io/docs-as-code/main/how-to/test_to_doc_links.html>`_).

How It Works
============

.. code-block:: text

    host (test process, pytest)                      QEMU guest (Linux or QNX)
    ┌──────────────────────────────┐   tap0      ┌─────────────────────────────────┐
    │ GrandMaster (raw socket) ────┼── gPTP ─────┼─▶ time_slave ──shm──▶ time_daemon│
    │                              │             │                         │       │
    │ test code ──── ssh ──────────┼─────────────┼─▶ vehicle_time_probe ◀──┘       │
    │   (target fixture, ITF)      │◀── JSON ────┼──  (score::time client API)     │
    └──────────────────────────────┘             └─────────────────────────────────┘

#. **Build.** ``qemu_itf_tests()`` in ``score/tests/common/qemu_tests.bzl`` defines a
   `score_itf <https://github.com/eclipse-score/itf>`_ ``py_itf_test`` for each target. The binaries under test
   (``time_slave``, ``time_daemon``, ``vehicle_time_probe``) go to ``/opt/bin`` and the configuration to
   ``/opt/etc``. On Linux they are packed into an ext4 disk (``target_opt_disk``) that the Ubuntu image from
   ``@score_os_images`` mounts on ``/opt``. On QNX they are built into the IFS boot image (``target_ifs``, see
   ``score/tests/common/qnx_aarch64/init.build``).
#. **Boot.** The ITF QEMU plugin boots the image and provides the ``target`` fixture, which runs commands on the
   guest over SSH (``target.execute()`` / ``target.execute_async()``). With ``--config=qemu`` the test runs under
   ``@score_itf//scripts:run_under_qemu``. This creates a private user and network namespace with its own
   ``tap0`` device, so you don't need sudo or a host tap device, and the grandmaster gets ``CAP_NET_RAW``.
#. **Grandmaster.** ``score/tests/common/gptp_grandmaster.py`` sends two-step Sync/Follow_Up on ``tap0``
   (125 ms by default) and answers the slave's Pdelay_Req. Its time is the host ``CLOCK_REALTIME`` plus an offset
   (``GM_OFFSET_NS``, 1 h), so a result that just echoes the guest's own clock cannot pass. Tests can change it
   while it runs: pause/resume, step, change the rate (``set_rate_ppm``), add link delay or correction fields,
   jump sequence ids, stop answering Pdelay, send Announce/Signaling, or corrupt the messages (``inject_fault``
   with the manglers ``truncate_to``, ``set_byte``, ``set_message_length``, ``set_origin_timestamp``,
   ``header_only``). It also records every frame it receives from the slave, so tests can check the slave's
   traffic.
#. **Time stack.** The ``time_stack`` fixture removes stale shared memory, starts ``time_slave`` (with
   ``GPTP_IFACE`` set to the guest interface), waits for ``TimeSlave initialized``, then starts ``time_daemon``.
   It stops both and cleans up afterwards. Tests that need a different configuration or start order use
   ``running_time_stack()`` / ``start_time_slave()`` from ``score/tests/common/helpers.py`` directly.
#. **Observation.** ``score/tests/common/vehicle_time_probe/main.cpp`` reads ``VehicleTime``,
   ``HighResSteadyTime`` and ``SteadyTime`` and prints one JSON object per sample (time values, status flags,
   ``reliable``, ``rate_deviation``, ...). With ``--events`` it also subscribes to notifications and prints
   ``EVENT {...}`` lines. ``ProbeStream`` (fixtures ``probe_stream`` / ``event_probe_stream``) runs the probe
   continuously at 20 ms and parses the output step by step. This makes short-lived states visible, such as a time
   jump flag that clears after two Syncs. ``sample_vehicle_time()`` takes one-off samples instead.

Fixtures
--------

The fixtures are defined in ``score/tests/common/fixtures.py`` and re-exported by each directory's
``conftest.py``.

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Fixture
     - Provides
   * - ``target``
     - (from ITF) the booted guest
   * - ``platform``
     - ``Platform`` entry for the guest OS (``uname -s``): interface lookup, shm paths
   * - ``gptp_iface``
     - name of the guest interface connected to the grandmaster
   * - ``time_stack``
     - running ``(time_slave, time_daemon)`` process handles
   * - ``probe_stream``
     - continuously sampling ``ProbeStream``
   * - ``event_probe_stream``
     - ``ProbeStream`` that also records notifications
   * - ``grandmaster``
     - running ``GrandMaster`` on ``tap0`` with the default offset
   * - ``jump_detection_armed``
     - synchronized stack where the 5 s start-up suppression of jump detection is over
   * - ``host_clock_check``
     - (session, used automatically) stops the run if the host clock is slewed against the hardware clock, see
       `Host Clock`_

Waiting for the guest is done with ``wait_for()``, which polls a condition and raises ``TimeoutError`` with
diagnostics (for example the latest probe sample) when the condition is not met in time.

Known Issues (Expected Failures)
--------------------------------

Some tests expose defects that are not fixed yet. Every such test is marked with an ``xfail`` marker from
``score/tests/common/known_issues.py``, and markers are defined only in that file. Each marker is named
``XFAIL_<issue>_<what is wrong>`` and its reason links the GitHub issue and, if there is one, the fixing pull
request.

- ``strict=True`` is for deterministic failures. When the defect is fixed the test reports ``XPASS(strict)`` and
  fails. Remove the marker from the test (and from ``known_issues.py`` if nothing else uses it).
- ``strict=False`` is for failures that depend on the platform, mostly the jitter of the software timestamps (the
  virtio NIC has no hardware timestamps). These tests can pass. The clock of the QEMU guest itself is accurate
  (a few ppm from the host), but a host clock that is slewed against the hardware clock adds a rate error of
  several percent, see `Host Clock`_.

To add a known issue, define the marker in ``known_issues.py`` under a ``# --- #<issue>: ...`` heading and apply
it as a decorator. If the test is expected to time out, pass ``raises=TimeoutError`` so that other errors still
fail.

Running the Tests
=================

Host Setup
----------

You need QEMU and, for the Linux target, access to ``/dev/kvm``. On Debian/Ubuntu (including the dev container):

.. code-block:: sh

   tools/qemu/setup_qemu_host.sh

The script installs the QEMU packages and adds you to the group that owns ``/dev/kvm``. If no group on the system
owns that GID, as can happen in a container, it creates a local ``kvm`` group with it. Log in again, or prefix
commands with ``sg kvm -c '...'``.

Commands
--------

The test targets are tagged ``integration``, ``qemu`` and ``manual``, so they are not part of ``//score/...``; run them by
name with ``--config=qemu``.

.. code-block:: sh

   # Feature tests on Linux x86_64 (KVM)
   sg kvm -c 'bazel test --config=qemu //score/tests/feature:test_feature_qemu_linux_x86_64'

   # Feature tests on QNX aarch64 (emulated, no KVM needed; takes much longer, timeout "eternal")
   bazel test --config=arm64-qnx --config=qemu //score/tests/feature:test_feature_qemu_qnx_aarch64

The QNX target needs the QNX SDP toolchain and license, like any other ``--config=arm64-qnx`` build.

Useful additions:

.. code-block:: sh

   # Run a subset of tests (pytest -k expression)
   --test_arg=-k --test_arg=startup

   # Show the test output live instead of only in the log
   --test_output=streamed

   # Rerun even if the inputs did not change
   --nocache_test_results

   # Run although the host clock is slewed against the hardware clock (see "Host Clock")
   --test_arg=--skip-host-clock-check

The pytest output is in ``bazel-testlogs/score/tests/feature/<target>/test.log``.

Troubleshooting
---------------

- **Permission denied on** ``/dev/kvm`` **, or a very slow boot / ping timeout on Linux:** you are not in the KVM
  group in this shell. Re-run with ``sg kvm -c '...'`` or log in again.
- ``PermissionError`` **when the grandmaster opens its socket:** you ran without ``--config=qemu``, so there is no
  namespace with ``CAP_NET_RAW`` and ``tap0``.
- **Platform-dependent failures (timing, rate, jumps):** check whether the test is covered by a non-strict marker
  in ``known_issues.py``. A loaded host makes the guest clock and the software timestamps noisier.
- **The run stops at the start with "The clock of the host is not running at the pace of the hardware clock", or
  many tests fail their drift check:** the host clock is slewed, see `Host Clock`_.

See also the general :ref:`manual_time_troubleshooting`.

Host Clock
----------

The grandmaster takes its time from the host's ``CLOCK_REALTIME``, while the clock of a KVM guest follows the
hardware clock. If something adjusts the tick or the frequency of the host clock, the two run at different rates.
Vehicle time then steps forward by a few milliseconds at every Sync, and the tests that compare vehicle time with
the pace of the grandmaster fail their drift check (``SYNC_TOLERANCE_NS``, 50 ms over a few seconds), for example
the tests in ``test_synchronization.py``.

The session fixture ``host_clock_check`` measures ``CLOCK_MONOTONIC`` against ``CLOCK_MONOTONIC_RAW`` of the host
for 5 s and stops the run if they differ by more than 0.5 %. A healthy host differs by far less than 0.05 %, the
most that an NTP client slews the clock. Pass ``--test_arg=--skip-host-clock-check`` to run anyway.

To check by hand, this prints the relative difference, which is near 0 on a healthy host and ``0.03`` for a host
clock that is 3 % fast:

.. code-block:: sh

   python3 -c 'import time as t; a=(t.clock_gettime(t.CLOCK_MONOTONIC), t.clock_gettime(t.CLOCK_MONOTONIC_RAW)); t.sleep(5); b=(t.clock_gettime(t.CLOCK_MONOTONIC), t.clock_gettime(t.CLOCK_MONOTONIC_RAW)); print((b[0]-a[0])/(b[1]-a[1])-1)'

``adjtimex --print`` (Debian/Ubuntu package ``adjtimex``) shows the tick, which is 10000 on a clock that is not
adjusted by the tick.

On WSL2, this was seen with ``systemd-timesyncd`` running in the distro: the tick of the VM moved by several
percent in both directions and the host clock ran up to about 4 % fast. WSL2 synchronizes the clock of its VM with
Windows itself, so the distro does not need an NTP client. Stopping the service made the clock stable and the
tests passed. Other causes are possible, so check that the clock is stable before looking at individual tests.

.. code-block:: sh

   systemctl is-active systemd-timesyncd
   sudo systemctl disable --now systemd-timesyncd

Stopping the service does not reset the tick that it left behind. Run ``wsl --shutdown`` in Windows afterwards, and
check the clock again after the restart.

Adding a Test
=============

#. Put it in ``feature/`` in the module for its topic, as ``test_<behaviour>()``. Decorate it
   with ``@add_test_properties(partially_verifies=[...], test_type=..., derivation_technique=...)`` (imported from
   ``attribute_plugin``), naming the requirement ID(s) it verifies. The docstring is mandatory, it is the description.
#. Use the fixtures above. For a non-default setup, create a
   ``GrandMaster(GM_IFACE, offset_ns=GM_OFFSET_NS, ...)`` or use ``running_time_stack(...)`` inside the test.
#. Wait with ``wait_for()`` / ``wait_for_stream*()`` instead of sleeping where you can, and compare against the
   tolerances and defaults in ``helpers.py`` (``SYNC_TOLERANCE_NS``, ``TIMEOUT_S``, ``JUMP_THRESHOLD_NS``, ...).
#. Make the test work on both Linux and QNX. QNX has no ``printf`` (use ``write_text_file()``), may lack
   ``/proc/uptime``, and its shared memory is in ``/dev/shmem``. Put platform differences in ``Platform`` in
   ``helpers.py``.
#. The BUILD files glob ``test_*.py``, so you don't need to change any BUILD file. If you deploy a new file to the
   guest, add it to ``target_binaries`` or ``target_config`` in ``score/tests/common/BUILD``.
