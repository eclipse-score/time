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

"""Compiler options for translation units using SharedMemoryHandler."""

# Preserve target-tuned cache-line spacing within a single system-image build.
# IPC peers must use the same toolchain, standard library and layout flags.
# See the shared-memory layout contract in the TimeDaemon detailed design.
# Each compiling target needs these options because copts do not propagate.
SHARED_MEMORY_HANDLER_COPTS = select({
    "@rules_cc//cc/compiler:gcc": ["-Wno-interference-size"],
    "//conditions:default": [],
})
