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
cc_test(
    name = "cpp_test_main",
    srcs = ["test_main.cpp"],
    deps = [
        "@googletest//:gtest_main",  # GoogleTest dependency via Bazel Modules
    ],
)

cc_test(
    name = "test_shared_state",
    srcs = ["test_shared_state.cpp"],
    copts = ["-std=c++17"],
    linkopts = ["-lpthread"],
    deps = [
        "//src/common:common_ipc",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_string_parser",
    srcs = ["test_string_parser.cpp"],
    copts = ["-std=c++17"],
    deps = [
        "//src/common:common_ipc",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_time_utils",
    srcs = ["test_time_utils.cpp"],
    copts = ["-std=c++17"],
    linkopts = ["-lrt"],
    deps = [
        "//src/common:common_ipc",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_pthread_lock_guard",
    srcs = ["test_pthread_lock_guard.cpp"],
    copts = ["-std=c++17"],
    linkopts = ["-lpthread"],
    deps = [
        "//src/common:common_ipc",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_config_loader",
    srcs = ["test_config_loader.cpp"],
    copts = ["-std=c++17"],
    linkopts = ["-lpthread"],
    deps = [
        "//src/tsyncd:config_loader_lib",
        "@googletest//:gtest_main",
    ],
)
