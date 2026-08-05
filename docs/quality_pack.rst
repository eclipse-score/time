Quality Pack Targets
####################

The ``score_time`` module plugs into the Score docs-as-code
dashboards and quality gates as described in the upstream how-to:
https://eclipse-score.github.io/docs-as-code/main/how-to/dashboards_and_quality_gates.html.

The Bazel targets below are the ones consumed by CI to produce
dashboard artefacts and to enforce traceability thresholds.

Unit tests
==========

- **Tag:** ``unit`` (already carried by every ``cc_test`` under
  ``//score/...``).
- **Command:** ``bazel test --config=time-x86_64-linux //score/...`` — this
  runs the full unit-test set because every ``cc_test`` in the tree
  carries the ``unit`` tag.
- **Results:** JUnit XML and stdout log per test target under
  ``bazel-testlogs/<package>/<test>/{test.log,test.xml}``.

Component tests
===============

Component tests exercise a clock facade (``Clock<Tag>``) together with a
mocked backend via ``ScopedClockOverride`` — the seam between the framework
layer and a domain-specific backend is covered end to end.

- **Tag:** ``component``.
- **Aggregate target:** ``//:component_tests``.
- **Command:** ``bazel test --config=time-x86_64-linux //:component_tests``.
- **Included tests (existing tests reclassified, not new ones):**

  - ``//score/time/vehicle_time/src:vehicle_clock_test``
  - ``//score/time/high_res_steady_time/src:high_res_steady_clock_test``
  - ``//score/time/system_time/src:system_clock_test``
  - ``//score/time/steady_time/src:steady_clock_test``

- **Results:** JUnit XML and stdout log per test target under
  ``bazel-testlogs/<package>/<test>/{test.log,test.xml}``.

Code coverage
=============

- **Command:** ``.github/tools/coverage.sh //score/... --config time-x86_64-linux``.
- **Underlying target:** ``bazel coverage`` with the ``coverage`` config
  from ``.bazelrc``.
- **Results:** HTML report at ``cpp_coverage/index.html`` and Cobertura
  XML at ``cpp_coverage/coverage.xml``. The raw ``lcov`` data lives under
  ``$(bazel info output_path)/_coverage/_coverage_report.dat``.
- **CI:** ``.github/workflows/code-coverage.yml`` runs the reusable
  ``eclipse-score/cicd-workflows`` coverage workflow with the same
  target and config and enforces the configured minimum coverage
  threshold.

Requirements traceability (dashboards + gate)
=============================================

Requirements live under ``docs/features/time/`` and use the Score
metamodel directives (``feat_req::`` / ``comp_req::``). Source-code
and test-code links are consumed by ``score_docs_as_code``:

- **Source-code markers** — in the C++ implementation:

  .. code-block:: cpp

     // # req-Id: comp_req__time__vehicle_clock_snapshot
     Snapshot Now() { ... }

  The leading ``// #`` is intentional; the linker regex looks for the
  literal token ``# req-Id:`` and this is the neutral C++ form. The
  files that carry markers are collected in
  ``//score/time/vehicle_time/src:requirement_marked_sources`` (a
  ``filegroup``) and passed to the root ``docs()`` macro via its
  ``scan_code`` attribute.

- **Test-code links** — use GoogleTest ``RecordProperty`` inside each
  linked test body:

  .. code-block:: cpp

     TEST(VehicleClockTest, InitForwardsToBackend)
     {
         ::testing::Test::RecordProperty("FullyVerifies", "comp_req__time__vehicle_clock_lifecycle");
         ::testing::Test::RecordProperty("TestType", "requirements-based");
         ::testing::Test::RecordProperty("DerivationTechnique", "requirements-analysis");
         ::testing::Test::RecordProperty("Description", "…");
         ...
     }

  The properties land in ``bazel-testlogs/.../test.xml`` and are read by
  ``score_source_code_linker`` when docs are built.

- **Bazel targets:**

  - ``//:needs_json`` — needs.json produced by Sphinx-Needs.
  - ``//:metrics_json`` — traceability metrics extracted from needs.json.
  - ``//:traceability_gate`` — enforces coverage thresholds.

- **Local flow** (order matters — the gate reads ``bazel-testlogs`` for
  test links):

  .. code-block:: bash

     bazel test --config=time-x86_64-linux //:component_tests //score/...
     bazel run  //:docs
     bazel run  //:traceability_gate -- \
       --metrics-json "$(pwd)/_build/metrics.json" \
       --need-type comp_req \
       --min-req-code 40 \
       --min-req-test 100 \
       --min-req-fully-linked 40 \
       --min-tests-linked 15

  Current baseline (component requirements only):

  =========================  ===============
  Metric                     Value
  =========================  ===============
  Requirements with source   2/5 (40.0%)
  Requirements with test     5/5 (100.0%)
  Requirements fully linked  2/5 (40.0%)
  Tests linked to reqs       5/27 (18.5%)
  =========================  ===============

.. note::

   The exact target names and result folders above are the current
   convention for this repository. They can be renamed together with
   ``@Zwinkau Andreas (ETAS-ECM ESY3)`` if a project-wide naming scheme
   is agreed upon; the CI workflows in ``.github/workflows`` reference
   these targets directly and would need to move in lockstep.
