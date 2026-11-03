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
    name = "phc_adjuster",
    srcs = select({
        "@platforms//os:qnx": ["//score/time_slave/src/gptp/platform/qnx:phc_adjuster_src"],
        "//conditions:default": ["//score/time_slave/src/gptp/platform/linux:phc_adjuster_src"],
    }),
    hdrs = ["phc_adjuster.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["QM"],
    visibility = ["//score:__subpackages__"],
    deps = select({
        "@platforms//os:qnx": ["//score/time_slave/src/gptp/details:raw_socket_impl"],
        "//conditions:default": [],
    }),
)
