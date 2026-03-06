# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "config_loader_lib",
    srcs = ["config/config_loader.cpp"],
    hdrs = [
        "config/config_loader.hpp",
        "daemon/tsync_types.hpp",
        "engine/time_sync_engine.hpp",
        "protocol/ntp_client.hpp",
    ],
    copts =
        ["-std=c++17"] +
        select({
            "@platforms//os:qnx": [
                "-D_QNX_PLAT",
                "-D_QNX_SOURCE",
                "-D__EXTENSIONS__",
            ],
            "//conditions:default": [],
        }),
    includes = [
        "config",
        "daemon",
        "engine",
        "protocol",
    ],
    deps = ["//src/common:common_ipc"],
)

cc_binary(
    name = "tsyncd",
    srcs =
        [
            "config/config_loader.cpp",
            "daemon/main.cpp",
            "engine/time_sync_engine.cpp",
            "protocol/eth_protocol.cpp",
            "protocol/gptp_protocol.cpp",
            "protocol/net_identity.cpp",
            "protocol/ntp_client.cpp",
            "protocol/raw_socket.cpp",
        ] +
        # QNX-only
        select({
            "@platforms//os:qnx": ["platform/qnx/qnx_raw_shim.cpp"],
            "//conditions:default": [],
        }) +
        select({
            "@platforms//os:linux": ["platform/linux/linux_raw_shim.cpp"],
            "//conditions:default": [],
        }) + glob([
            "daemon/*.hpp",
            "engine/*.hpp",
            "protocol/*.hpp",
            "config/*.hpp",
            "platform/qnx/*.hpp",
            "platform/linux/*.hpp",
        ]),
    copts =
        ["-std=c++17"] +
        select({
            "@platforms//os:qnx": [
                "-D_QNX_PLAT",
                "-D_QNX_SOURCE",
                "-D__EXTENSIONS__",
            ],
            "//conditions:default": [],
        }),
    data = [
        "config/tsyncd.conf",
    ],
    includes = [
        "config",
        "daemon",
        "engine",
        "platform/linux",
        "platform/qnx",
        "protocol",
    ],
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsocket",
            "-lc",
            "-lc++",
        ],
        "//conditions:default": [
            "-lrt",
            "-lpthread",
            "-lstdc++",
            "-lsupc++",
        ],
    }),
    deps = [
        "//src/common:common_ipc",
        "//src/libscore_time:score_time",
    ],
)
