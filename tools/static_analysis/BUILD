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

# MODULE-SPECIFIC: instantiates the shared CodeQL analysis for this module.
# Other modules can reuse codeql.bzl, codeql_lint.py, and config.yaml by
# copying those three files and providing their own coding-standards.yaml.
load(":codeql.bzl", "codeql_analysis")

# MODULE-SPECIFIC: declares the MISRA deviation file as an explicit Bazel
# target so that incremental rebuilds detect changes to it.
filegroup(
    name = "coding_standards",
    srcs = ["coding-standards.yaml"],
)

codeql_analysis(
    coding_standards = ":coding_standards",
)
