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

# MODULE-AGNOSTIC: this file (together with codeql_lint.py and config.yaml)
# contains no module-specific knowledge and can be reused by other modules.
# To adopt: copy these three files, provide your own coding-standards.yaml,
# and call codeql_analysis() from your BUILD.

def codeql_analysis(
        name = "codeql_lint",
        coding_standards = "//tools/static_analysis:coding_standards"):
    """Instantiates the CodeQL MISRA C++ analysis binary for a module.

    Args:
        name: name of the resulting py_binary target.
        coding_standards: label of the coding-standards.yaml deviation file.
            Defaults to //:coding-standards.yaml, the workspace-root location
            scanned by process_coding_standards_config.
    """
    _config_name = name + "_config"

    native.filegroup(
        name = _config_name,
        srcs = [Label("//tools/static_analysis:config.yaml")],
    )

    native.py_binary(
        name = name,
        srcs = [Label("//tools/static_analysis:codeql_lint.py")],
        args = [
            "--codeql_path=$(location @codeql_bundle//:codeql_cli)",
            "--config_path=$(location :" + _config_name + ")",
            "--analysis_report_path=$(location @codeql_coding_standards//:analysis_report)",
        ],
        data = [
            ":" + _config_name,
            coding_standards,
            "@codeql_bundle//:codeql_cli",
            "@codeql_coding_standards//:analysis_report",
            "@codeql_coding_standards//:process_coding_standards_config",
            "@codeql_coding_standards_compiled//:pack",
        ],
        main = Label("//tools/static_analysis:codeql_lint.py"),
        tags = ["local"],
        target_compatible_with = ["@platforms//os:linux"],
        deps = [Label("@rules_python//python/runfiles")],
    )
