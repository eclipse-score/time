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

load("@aspect_rules_py//py:defs.bzl", "py_venv")
load("@pip_time_daemon_venv//:requirements.bzl", "all_requirements")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")

# To regenerate requirements.txt.lock after editing requirements.txt, run:
#   bazel run //score/time_daemon/tests/component_tests:requirements.update
compile_pip_requirements(
    name = "requirements",
    srcs = [
        "requirements.txt",
        "@score_tooling//python_basics:requirements.in",
    ],
    requirements_txt = "requirements.txt.lock",
    tags = ["manual"],
)

# Host-only developer venv: no Python toolchain exists for QNX target platforms.
# Uses py_venv directly because score_virtualenv does not forward
# target_compatible_with.
py_venv(
    name = "python_tc_venv",
    data = ["@score_tooling//python_basics:pyproject.toml"],
    package_collisions = "warning",
    target_compatible_with = ["@platforms//os:linux"],
    venv_name = ".python_tc_venv",
    deps = all_requirements + ["@rules_python//python/runfiles"],
)

# NOTE: The time_daemon_cit pytest target is intentionally absent.
#
# Previous CIT scenarios (lifecycle, shm_roundtrip, pipeline) exercised
# component-internal behaviour inside a single process — those belong in
# gtest-based unit/component integration tests under src/.
#
# The CIT framework (Python driver + test_scenarios binary) is reserved
# for testing the TimeDaemon as an external binary, with true IPC between
# processes.  Re-enable score_py_pytest here once a real process-level
# integration scenario is added.
