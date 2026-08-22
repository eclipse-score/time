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


class TestControlFlowPtpDivider(TimeDaemonCitScenario):
    """Verifies PtpControlFlowDivider routes a PtpTimeInfo message to its publish callback.

    A single PtpTimeInfo message is injected via OnMessage().  The divider runs
    on its own EventDrivenMachine worker thread; a promise/future pair in the C++
    scenario binary synchronises the result.  The test confirms that
    ptp_assumed_time and is_synchronized pass through unchanged.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "control_flow.ptp_divider"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — divider processed the message."""
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_ptp_divider_active(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm the scenario ran to completion."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_divider_active", value="true")

    def test_ptp_time_preserved_through_divider(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """ptp_assumed_time must be forwarded by the divider without modification.

        PtpControlFlowDivider is a transparent relay; it must not alter the
        ptp_assumed_time of any message it forwards to its publish callback.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_time_preserved", value="true")

    def test_sync_status_preserved_through_divider(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_synchronized must be forwarded by the divider without modification.

        PtpControlFlowDivider must not alter PtpStatus fields; in particular,
        is_synchronized must match the value set in the input message.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="sync_status_preserved", value="true")
