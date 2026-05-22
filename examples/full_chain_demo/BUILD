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

cc_binary(
    name = "fake_time_slave",
    srcs = ["fake_time_slave.cpp"],
    features = COMPILER_WARNING_FEATURES,
    deps = [
        "//score/ts_client/src:gptp_ipc",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

# Process 2: standalone TimeDaemon for testing — no lifecycle manager required.
# NOTE: Cannot be built from this package because svt_handler_shm has
#   visibility = ["//score/time_daemon/src/application:__pkg__"]
# Build target lives at //score/time_daemon/src/application:demo_time_daemon
# The source is kept here for reference alongside the other demo processes.
#
# cc_binary(
#     name = "demo_time_daemon",
#     srcs = ["demo_time_daemon.cpp"],
#     features = COMPILER_WARNING_FEATURES,
#     deps = [
#         "//score/time_daemon/src/application/svt:svt_handler_shm",
#         "//score/time_daemon/src/common:logging_contexts",
#         "@score_baselibs//score/concurrency",
#         "@score_baselibs//score/mw/log:console_only_backend",
#     ],
# )

# NOTE: demo_app depends on //score/time/SynchronizedVehicleTime which is not
# owned by this package. Update include paths in demo_app.cpp and the dep below
# once the score/time package layout is confirmed, then uncomment.
#
# cc_binary(
#     name = "demo_app",
#     srcs = ["demo_app.cpp"],
#     features = COMPILER_WARNING_FEATURES,
#     deps = [
#         "//score/time/SynchronizedVehicleTime:SynchronizedVehicleTime",
#         "@score_baselibs//score/mw/log:console_only_backend",
#     ],
# )
