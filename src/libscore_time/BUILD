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
    name = "score_time",
    srcs = [
        "time_client/score_time.cpp",
    ],
    hdrs = glob(["include/**/*.hpp"]),
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
    include_prefix = "score_time",
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
    strip_include_prefix = "include",
    deps = [
        "//src/common:common_ipc",
    ],
)
