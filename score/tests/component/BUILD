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

load("//score/tests/common:qemu_tests.bzl", "qemu_itf_tests")

# Tests of the component requirements. They are skipped unless run with
#   --test_arg=--run-component-requirement-tests
qemu_itf_tests(
    name = "test_component_qemu",
    srcs = ["conftest.py"] + glob(["test_*.py"]),
)
