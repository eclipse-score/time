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

load("@score_tools//codeql:defs.bzl", "codeql_sca")

# Linux analysis. Test/mock packages are excluded automatically.
codeql_sca(
    name = "sca_linux",
    configs = ["x86_64-linux"],  # production GCC 12.2 toolchain; no LLVM
    safety_level = "ASIL",
    targets = ["//score/..."],
)

# QNX analysis (run in CI only; needs the QNX SDP + license).
codeql_sca(
    name = "sca_qnx",
    configs = ["x86_64-qnx"],
    platform = "qnx",
    safety_level = "ASIL",
    targets = ["//score/..."],
)
