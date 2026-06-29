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

cc_library(
    name = "stub_ptp_engine",
    srcs = [
        "stub_ptp_engine.cpp",
    ],
    hdrs = [
        "stub_ptp_engine.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    includes = [],
    tags = ["QM"],
    visibility = ["//score/time_daemon:__subpackages__"],
    deps = [
        "//score/time/high_res_steady_time:interface",
        "//score/time_daemon/src/common:logging_contexts",
        "//score/time_daemon/src/common/data_types:ptp_time_info",
        "@score_baselibs//score/mw/log:frontend",
    ],
)
