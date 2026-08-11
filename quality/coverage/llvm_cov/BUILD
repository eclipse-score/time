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

load("@rules_cc//cc/toolchains:args.bzl", "cc_args")
load("@rules_cc//cc/toolchains:feature.bzl", "cc_feature")
load("@rules_python//python:defs.bzl", "py_binary")
load("//quality/coverage/llvm_cov:reporter_wrapper.bzl", "reporter_wrapper")

# Enables LLVM runtime counter relocation — required for LLVM_PROFILE_CONTINUOUS_MODE.
# Without this, continuous-mode profiling from ASSERT_DEATH subprocesses fails.
cc_args(
    name = "runtime_relocation_args",
    actions = [
        "@rules_cc//cc/toolchains/actions:compile_actions",
        "@rules_cc//cc/toolchains/actions:link_actions",
    ],
    args = [
        "-mllvm",
        "-runtime-counter-relocation",
    ],
)

cc_feature(
    name = "enable_llvm_coverage_for_death_tests",
    args = [":runtime_relocation_args"],
    feature_name = "enable_llvm_coverage_for_death_tests",
    visibility = ["//visibility:public"],
)

py_binary(
    name = "merger",
    srcs = ["merger.py"],
)

py_binary(
    name = "reporter",
    testonly = True,
    srcs = ["reporter.py"],
    data = [
        "@llvm_toolchain//:llvm-cov",
        "@llvm_toolchain//:llvm-profdata",
        "@llvm_toolchain_llvm//:bin/llvm-cxxfilt",
    ],
    deps = ["@rules_python//python/runfiles"],
)

reporter_wrapper(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = "//quality/coverage:time_coverage_scope",
    reporter = ":reporter",
)
