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

load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

# Test-only stub: provides CreateBackend<std::chrono::system_clock>() returning
# an epoch-zero snapshot. Link this (via :system_clock_mock) into test binaries
# instead of the production system_time_impl backend.
cc_library(
    name = "stub_impl",
    testonly = True,
    srcs = ["system_clock_backend_impl.cpp"],
    hdrs = ["system_clock_backend_impl.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time/system_time:__subpackages__"],
    deps = [
        "//score/time/clock:clock_core",
        "//score/time/system_time/src:system_clock",
    ],
)
