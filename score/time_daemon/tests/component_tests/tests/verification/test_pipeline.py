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


class TestVerificationPipeline(TimeDaemonCitScenario):
    """Verifies that PtpTimeInfo flows correctly through the full verification pipeline.

    GPTPStubMachine (PTP source) is wired to SvtVerificationMachine (three-stage validator).
    The test confirms that data produced by the stub reaches and exits the pipeline with
    the expected status fields, exercising the SynchronizationValidator, TimeoutValidator,
    and TimeJumpsValidator stages in a real component integration.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "verification.pipeline"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — pipeline initialised and data received."""
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_pipeline_reached_active_state(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm the pipeline processed at least one data point."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="pipeline_active", value="true")

    def test_verification_reports_synchronized(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """SynchronizationValidator must propagate is_synchronized=true from stub data."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_synchronized", value="true")

    def test_verification_reports_correct_status(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Verified data must carry is_correct=true — no validator flagged an anomaly."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_correct", value="true")

    def test_verification_no_timeout(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TimeoutValidator must not trigger a timeout on the first data point from stub."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_timeout", value="false")

    def test_verification_positive_timestamp(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """PTP assumed time must remain positive after passing through the pipeline."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_time_positive", value="true")
