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
from typing import Any

import pytest
from cit_scenario import TimeDaemonCitScenario
from result_code import ResultCode
from testing_utils import LogContainer, ScenarioResult


class TestVerificationTimeJumpDetection(TimeDaemonCitScenario):
    """Verifies TimeJumpsValidator forward-jump detection inside SvtVerificationMachine.

    PtpTimeInfo frames are injected directly (no GPTPStubMachine).  The scenario
    must first complete the initial sync debouncing phase (5 s wall clock +
    2 synchronized frames) before TimeJumpsValidator becomes active.  It then
    injects a ptp_assumed_time jump of 1 ms, which exceeds max_time_jump_allowed
    (500 µs) and must raise is_time_jump_future in the pipeline output.

    Note: this test takes ~6 s to run due to the 5 s sync debounce threshold.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "verification.time_jump_detection"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — all phases completed."""
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_time_jump_detection_active(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm the scenario ran to completion."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="time_jump_detection_active", value="true"
        )

    def test_forward_time_jump_raises_is_time_jump_future(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """A ptp_assumed_time jump of 1 ms (> 500 µs threshold) must set is_time_jump_future=true.

        After TimeJumpsValidator exits kInitialSyncDebouncing, a forward jump in
        ptp_assumed_time that exceeds max_time_jump_allowed must be detected and
        surfaced in the output PtpStatus.is_time_jump_future field.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="time_jump_future_detected", value="true"
        )
