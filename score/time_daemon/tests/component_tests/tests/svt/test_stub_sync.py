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


class TestStubPublishesData(TimeDaemonCitScenario):
    """Verifies that GPTPStubMachine initializes correctly and publishes synchronized PTP time data."""

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "svt.stub_publishes_data"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0."""
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_stub_reports_synchronized(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Stub PTP machine must report is_synchronized=true on first publication."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_synchronized", value="true")

    def test_stub_reports_correct_status(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Stub PTP machine must report is_correct=true on first publication."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_correct", value="true")

    def test_stub_no_timeout(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Stub PTP machine must report is_timeout=false (stub never times out)."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_timeout", value="false")

    def test_stub_positive_timestamp(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Published PTP assumed time must be a positive nanosecond value."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_time_positive", value="true")
