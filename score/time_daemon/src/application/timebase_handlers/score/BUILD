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
    name = "score_timebase_handler_factory",
    srcs = ["score_timebase_handler_factory.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/time_daemon/src/application:__pkg__"],
    deps = [
        "//score/time_daemon/src/application/timebase_handlers:timebase_handler_factory",
        "//score/time_daemon/src/application/timebase_handlers/score/vehicle_time:score_vehicle_timebase_handler",
    ],
    alwayslink = True,
)
