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
    name = "factory",
    srcs = ["factory.cpp"],
    hdrs = [
        "factory.h",
        "svt_receiver.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/ipc:__subpackages__"],
    deps = [
        "//score/time_daemon/src/ipc/core:receiver_impl",
        "//score/time_daemon/src/ipc/svt:config",
        "//score/time_daemon/src/ipc/svt:time_info",
    ],
)

cc_library(
    name = "factory_mock",
    testonly = True,
    srcs = ["factory_stub.cpp"],
    hdrs = [
        "factory.h",
        "svt_receiver.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/ipc:__subpackages__"],
    deps = [
        "//score/time_daemon/src/ipc:receiver_mock",
        "//score/time_daemon/src/ipc/svt:config",
        "//score/time_daemon/src/ipc/svt:time_info",
    ],
)
