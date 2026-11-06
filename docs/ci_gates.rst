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

CI validation gates
===================

Time defines two stable CI checks:

* ``ci/general-validation`` from the ``Gate / General checks`` workflow in
  ``.github/workflows/gate_general_checks.yml``.
* ``ci/qnx-validation`` from the ``Gate / QNX builds`` workflow in
  ``.github/workflows/gate_qnx_builds.yml``.

The central repository policy is intended to use these aggregate checks as its
CI requirements. ECA, reviews, and merge-queue rules are separate protections.
The check names are the stable interface; Time owns their internal dependency
graphs. The general gate combines multiple reusable workflows, while the QNX
gate aggregates jobs within the existing QNX workflow.

General checks
--------------

The ``Gate / General checks`` workflow runs for ``pull_request`` and
``merge_group: checks_requested``. It blocks on:

* x86_64 Linux and ARM64 Linux builds of ``//score/...`` and ``//examples/...``.
* Component tests using ``test --config=x86_64-linux //score/...``.
* LLVM code coverage, including the existing 85% threshold.
* Documentation verification and build.
* All-language formatting.
* Copyright validation.
* Bzlmod lockfile and tidy checks.

It calls the existing Linux, component-test, coverage, formatting, copyright,
and bzlmod producers. Documentation uses the existing pinned shared workflows.
The existing documentation/quality-pack publication workflow remains separate.

The aggregate result job always runs after its dependencies. Every dependency
must succeed; a failure, cancellation, or unexpected skip fails the aggregate.

QNX builds
----------

The ``Gate / QNX builds`` workflow runs for ``pull_request_target`` and
``merge_group: checks_requested``. It also retains its ``workflow_call`` entry
point for cache warm-up.

On ``merge_group``, the shared QNX producer builds both ``x86_64-qnx`` and
``arm64-qnx`` for ``//score/... //examples/...``. The matrix runs both
architectures even if one fails. The existing ``QNX merge queue result`` job
succeeds only when both queue builds succeed.

On ``pull_request_target``, the queue builds intentionally skip and
``QNX merge queue result`` reports success. Advisory PR builds remain
independent, so neither the queue result nor the aggregate waits for fork-QNX
environment approval.

The ``ci/qnx-validation`` job uses ``always()`` and depends only on
``QNX merge queue result``. It requires that result to succeed and rejects
failure, cancellation, or skip. It runs only for ``pull_request_target`` and
``merge_group``, not for cache warm-up calls on ``push`` or
``workflow_dispatch``. The aggregate job itself does not check out PR code and
has no token permissions. The QNX gate covers builds, not QNX tests.

Aggregate contents
------------------

.. list-table:: Stable CI checks and their blocking validation
   :header-rows: 1
   :widths: 32 68

   * - Required CI check
     - Blocking validation
   * - ``ci/general-validation``
     - Both Linux architecture builds
   * - ``ci/general-validation``
     - x86_64 Linux component tests
   * - ``ci/general-validation``
     - LLVM code coverage and its 85% threshold
   * - ``ci/general-validation``
     - Documentation verification and build
   * - ``ci/general-validation``
     - All-language formatting
   * - ``ci/general-validation``
     - Copyright, bzlmod lockfile, and bzlmod tidy checks
   * - ``ci/qnx-validation``
     - Both QNX architecture builds on the merge-group revision

Other non-required checks retain their existing policy and ownership.

Reusable workflow isolation
---------------------------

The existing validation workflows also serve their direct PR, push, and
merge-group triggers. Their ``workflow_call`` entry points let the general gate
reuse those implementations rather than duplicate build/test logic.

GitHub evaluates a called workflow's ``github.workflow`` as the caller's
workflow name. Each reusable producer therefore adds a distinct suffix to its
concurrency group; without it, ``cancel-in-progress`` could make a called
workflow cancel its caller or a sibling producer for the same ref.

The QNX workflow owns both its build jobs and aggregate result. Its reusable
entry point is used by cache maintenance, not by a second aggregate workflow.
This keeps a single queue-build matrix and preserves the existing build/result
check names and cache settings.
