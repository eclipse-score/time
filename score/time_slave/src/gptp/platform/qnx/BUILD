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

# Source files carrying `// req-Id:` markers, exposed to
# //score/time_slave:docs_bundle via its code_targets attribute.
filegroup(
    name = "requirement_marked_sources",
    srcs = [
        "phc_adjuster.cpp",
        "qnx_raw_shim.cpp",
        "raw_socket.cpp",
    ],
    visibility = ["//score/time_slave:__pkg__"],
)

filegroup(
    name = "raw_socket_src",
    srcs = [
        "qnx_raw_shim.cpp",
        "raw_socket.cpp",
    ],
    visibility = ["//score:__subpackages__"],
)

filegroup(
    name = "network_identity_src",
    srcs = ["network_identity.cpp"],
    visibility = ["//score:__subpackages__"],
)

filegroup(
    name = "phc_adjuster_src",
    srcs = ["phc_adjuster.cpp"],
    visibility = ["//score:__subpackages__"],
)
