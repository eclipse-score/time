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

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")
load("@rules_python//python:defs.bzl", "py_library")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")
load("@score_rules_imagefs//rules/linux:ext4.bzl", "ext4")
load("@score_rules_imagefs//rules/qnx:ifs.bzl", "qnx_ifs")

package(default_visibility = ["//score/tests:__subpackages__"])

exports_files(["qnx_aarch64/qemu_config.json"])

# Helpers, known issues (xfail markers), fixtures and the gPTP grandmaster of the end-to-end tests. `imports` makes the modules importable
# by their plain names (e.g. `from helpers import wait_for`).
py_library(
    name = "e2e_support",
    srcs = [
        "fixtures.py",
        "gptp_grandmaster.py",
        "helpers.py",
        "known_issues.py",
    ],
    imports = ["."],
)

# Application that requests vehicle time from the time daemon and prints it as JSON.
cc_binary(
    name = "vehicle_time_probe",
    srcs = ["vehicle_time_probe/main.cpp"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["manual"],
    deps = [
        "//score/time/high_res_steady_time",
        "//score/time/steady_time",
        "//score/time/vehicle_time",
        "@score_baselibs//score/mw/log:console_only_backend",
    ],
)

# Everything that is deployed to the target: /opt/bin/{time_slave,time_daemon,vehicle_time_probe}
# and the configuration in /opt/etc.
pkg_files(
    name = "target_binaries",
    srcs = [
        ":vehicle_time_probe",
        "//score/time_daemon",
        "//score/time_slave",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "bin",
    tags = ["manual"],
)

pkg_files(
    name = "target_config",
    srcs = [
        "config/logging.json",
        "config/time_slave_config.json",
    ],
    prefix = "etc",
    tags = ["manual"],
)

# The same files below /opt, for images that are not extended by a disk.
pkg_filegroup(
    name = "target_opt_files",
    srcs = [
        ":target_binaries",
        ":target_config",
    ],
    prefix = "/opt",
    tags = ["manual"],
)

# Additional disk that the ITF Ubuntu image mounts on /opt.
ext4(
    name = "target_opt_disk",
    srcs = [
        ":target_binaries",
        ":target_config",
    ],
    out = "target_opt.ext4",
    tags = ["manual"],
    target_compatible_with = ["@platforms//os:linux"],
)

# QNX image with the binaries under test in /opt.
qnx_ifs(
    name = "target_ifs",
    srcs = [":target_opt_files"],
    build_file = "qnx_aarch64/init.build",
    tags = ["manual"],
    target_compatible_with = [
        "@platforms//cpu:aarch64",
        "@platforms//os:qnx",
    ],
)
