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

load("@score_docs_as_code//:docs.bzl", "docs")

docs(
    bundles = [
        {
            "bundle": "//score/time_slave:docs_bundle",
            "mount_at": "time_slave",
            "attach_to": "module/index",
        },
        {
            "bundle": "//score/time_daemon:docs_bundle",
            "mount_at": "time_daemon",
            "attach_to": "module/index",
        },
        {
            "bundle": "//score/ts_client:docs_bundle",
            "mount_at": "ts_client",
            "attach_to": "module/index",
        },
        {
            "bundle": "//score/time:docs_bundle",
            "mount_at": "time",
            "attach_to": "module/index",
        },
    ],
    external_needs = [
        "@score_process_description//:needs_json_file",
        "@score_platform//:needs_json_file",
    ],
    project = "S-CORE Time",
    project_url = "https://eclipse-score.github.io/time",
    source_dir = "docs",
)

exports_files(
    [
        # Used by the @score_tooling coverage reporter to locate the workspace root.
        "MODULE.bazel",
        # Local clang-tidy overrides, layered on top of the S-CORE baseline.
        ".clang-tidy",
    ],
)
