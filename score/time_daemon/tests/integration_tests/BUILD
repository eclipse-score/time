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

load("@pip_time_daemon_venv//:requirements.bzl", "all_requirements")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_tooling//python_basics:defs.bzl", "score_py_pytest", "score_virtualenv")

# To regenerate requirements.txt.lock after editing requirements.txt, run:
#   bazel run //score/time_daemon/tests/integration_tests:requirements.update
compile_pip_requirements(
    name = "requirements",
    srcs = [
        "requirements.txt",
        "@score_tooling//python_basics:requirements.txt",
    ],
    requirements_txt = "requirements.txt.lock",
    tags = ["manual"],
)

score_virtualenv(
    name = "python_tc_venv",
    reqs = all_requirements,
    venv_name = ".python_tc_venv",
)

score_py_pytest(
    name = "time_daemon_cit",
    srcs = glob(["tests/**/*.py"]) + ["conftest.py"],
    args = [
        "--cpp-target-path=$(rootpath //score/time_daemon/tests/test_scenarios/cpp:test_scenarios)",
    ],
    data = [
        ":python_tc_venv",
        "//score/time_daemon/tests/test_scenarios/cpp:test_scenarios",
    ],
    pytest_config = ":pytest.ini",
    tags = ["integration"],
    deps = all_requirements,
)
