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

"""Macro for the end-to-end tests that run in QEMU, on Linux x86_64 and QNX aarch64."""

load("@score_itf//:defs.bzl", "py_itf_test")

def qemu_itf_tests(name, srcs):
    """Defines the end-to-end tests `srcs` for Linux x86_64 (<name>_linux_x86_64) and QNX aarch64 (<name>_qnx_aarch64).

    Both need CAP_NET_RAW for the grandmaster, hence run them with --config=qemu. They are tagged manual, so they
    are not part of //score/... and are run by naming them:
      sg kvm -c 'bazel test --config=qemu //score/tests/<dir>:<name>_linux_x86_64'
    QNX is emulated, so it needs no KVM but boots slowly:
      bazel test --config=arm64-qnx --config=qemu //score/tests/<dir>:<name>_qnx_aarch64

    Args:
      name: prefix of the test target names.
      srcs: the conftest.py and test modules.
    """
    py_itf_test(
        name = name + "_linux_x86_64",
        timeout = "long",
        srcs = srcs,
        args = [
            "--qemu-rootfs=$(location @score_os_images//ubuntu_x86_64:image)",
            "--qemu-config=$(location @score_os_images//ubuntu_x86_64:qemu_config)",
            "--qemu-disk=$(location //score/tests/common:target_opt_disk)",
        ],
        data = [
            "//score/tests/common:target_opt_disk",
            "@score_os_images//ubuntu_x86_64:image",
            "@score_os_images//ubuntu_x86_64:qemu_config",
            # The generated qemu_config.json points at this key (pkey_path).
            "@score_os_images//ubuntu_x86_64:ssh_target_key",
        ],
        deps = ["//score/tests/common:e2e_support"],
        plugins = [
            "@score_itf//score/itf/plugins:attribute_plugin",
            "@score_itf//score/itf/plugins:qemu_plugin",
        ],
        tags = [
            "integration",
            # Not part of //score/...: needs --config=qemu (ext4 toolchain, CAP_NET_RAW and tap0).
            "manual",
            "qemu",
        ],
        target_compatible_with = [
            "@platforms//cpu:x86_64",
            "@platforms//os:linux",
        ],
    )

    py_itf_test(
        name = name + "_qnx_aarch64",
        timeout = "eternal",  # QNX is emulated, the whole suite takes longer than 15 minutes
        srcs = srcs,
        args = [
            "--qemu-kernel=$(location //score/tests/common:target_ifs)",
            "--qemu-config=$(location //score/tests/common:qnx_aarch64/qemu_config.json)",
        ],
        data = [
            "//score/tests/common:qnx_aarch64/qemu_config.json",
            "//score/tests/common:target_ifs",
        ],
        deps = ["//score/tests/common:e2e_support"],
        plugins = [
            "@score_itf//score/itf/plugins:attribute_plugin",
            "@score_itf//score/itf/plugins:qemu_plugin",
        ],
        tags = [
            "integration",
            # Not part of //score/...: needs --config=qemu (ext4 toolchain, CAP_NET_RAW and tap0).
            "manual",
            "qemu",
        ],
        target_compatible_with = [
            "@platforms//cpu:aarch64",
            "@platforms//os:qnx",
        ],
    )
