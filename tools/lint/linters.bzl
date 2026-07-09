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

"""Clang-tidy aspect for the score_time module.

Uses the S-CORE centralized clang-tidy policy from score_cpp_policies,
which pre-wires the S-CORE baseline .clang-tidy config and aspect defaults.
"""

load("@score_cpp_policies//clang_tidy:defs.bzl", "make_clang_tidy_aspect", "make_clang_tidy_test")

clang_tidy_aspect = make_clang_tidy_aspect(
    binary = Label("@llvm_toolchain//:clang-tidy"),
    # No local_configs: use only the S-CORE baseline from score_cpp_policies.
)

clang_tidy_test = make_clang_tidy_test(aspect = clang_tidy_aspect)
