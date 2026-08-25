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

load("@score_tooling//coverage:defs.bzl", "score_coverage_reporter", "score_coverage_scope")

# The production components of this module (the public faces the old
# cpp-coverage workflow also targeted via //score/...). The scope aspect walks
# their transitive in-workspace deps; test suites, mocks (*_mock,
# *_test_utils) and external deps are not reachable from these roots and stay
# out of the report. Untested in-scope files appear at exact 0% via the
# --empty-profile baseline. Keep this list in sync with the module's shipped
# components.
score_coverage_scope(
    name = "coverage_scope",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [
        # Standalone public clock APIs, not reachable through the daemons:
        "//score/time/steady_time",
        "//score/time/system_time",
        "//score/time/vehicle_time",
        # The daemons transitively bring in clock_core, high_res_steady_clock
        # and ts_client's gptp_ipc (verified via bazel query deps()).
        # KNOWN GAP: the cc_binary entry-point sources themselves (main.cpp,
        # time_daemon.cpp, time_slave.cpp) are not collected — cc_binary
        # provides no CcInfo for the scope aspect. Entry-point wiring is not
        # unit-testable; tracked as a scope-aspect enhancement in score_tooling.
        "//score/time_daemon",
        "//score/time_slave",
    ],
)

score_coverage_reporter(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = ":coverage_scope",
    llvm_cov = "@llvm_toolchain_coverage//:llvm-cov",
    llvm_cxxfilt = "@llvm_toolchain_coverage_llvm//:bin/llvm-cxxfilt",
    llvm_profdata = "@llvm_toolchain_coverage//:llvm-profdata",
)
