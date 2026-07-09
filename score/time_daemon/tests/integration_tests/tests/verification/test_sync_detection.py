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


class TestVerificationSyncDetection(TimeDaemonCitScenario):
    """Verifies SynchronizationValidator latching behavior inside SvtVerificationMachine.

    PtpTimeInfo objects are injected directly (no GPTPStubMachine) to test the
    SynchronizationValidator state machine in isolation from async PTP traffic:
      1. Unsynchronized input  → is_synchronized=false in output (no latch yet).
      2. Synchronized input    → is_synchronized=true in output (latch fires).
      3. Unsynchronized input  → is_synchronized=true in output (latch holds).
    This cross-validates the three-stage pipeline without relying on stub timing.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "verification.sync_detection"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — all three injections processed."""
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_sync_detection_active(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm the scenario ran to completion."""
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="sync_detection_active", value="true")

    def test_unsynchronized_input_passes_through_as_unsynchronized(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """First message with is_synchronized=false must exit the pipeline unchanged.

        Before the SynchronizationValidator latches, it must not force
        is_synchronized=true on incoming unsynchronized data.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="unsync_output_is_synchronized", value="false"
        )

    def test_synchronized_input_exits_synchronized(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """First synchronized message must exit the pipeline with is_synchronized=true.

        The SynchronizationValidator must record synchronization and allow the
        flag to propagate to downstream consumers.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="sync_output_is_synchronized", value="true"
        )

    def test_sync_latch_holds_for_subsequent_unsynchronized_input(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """After latching, unsynchronized input must be forced to is_synchronized=true.

        Once the SynchronizationValidator has observed a synchronized frame,
        it must permanently override is_synchronized=true on all future messages,
        preventing transient PTP dropouts from propagating false unsync status.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="latch_output_is_synchronized", value="true"
        )
