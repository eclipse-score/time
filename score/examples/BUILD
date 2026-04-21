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

# Demo programs for board-level functional verification of TimeSlave + libTSClient.
#
# Typical workflow (three terminals on the board):
#   Terminal 1:  ./gptp_master  [iface]          — test gPTP master
#   Terminal 2:  ./timeslave_standalone [iface]   — gPTP slave + shared-memory publisher
#   Terminal 3:  ./time_reader                    — read and display shared-memory data

cc_binary(
    name = "time_reader",
    srcs = ["time_reader.cpp"],
    deps = [
        "//score/libTSClient:gptp_ipc",
    ],
)

cc_binary(
    name = "timeslave_standalone",
    srcs = ["timeslave_standalone.cpp"],
    deps = [
        "//score/TimeSlave/code/gptp:gptp_engine",
        "//score/libTSClient:gptp_ipc",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

cc_binary(
    name = "gptp_master",
    srcs = ["gptp_master.cpp"],
    target_compatible_with = ["@platforms//os:qnx"],
    linkopts = ["-lsocket"],
    deps = [
        "//score/TimeSlave/code/gptp/details:ptp_types",
    ],
)
