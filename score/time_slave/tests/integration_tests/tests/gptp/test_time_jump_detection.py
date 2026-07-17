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
from cit_scenario import TimeSlaveGptpCitScenario
from result_code import ResultCode
from testing_utils import LogContainer, ScenarioResult


class TestGptpTimeJumpDetection(TimeSlaveGptpCitScenario):
    """Verifies that GptpEngine sets is_time_jump_future=true and clears is_correct
    when two successive Sync+FollowUp pairs show a master-time delta that exceeds
    jump_future_threshold_ns (default 500 ms).

    Frame sequence injected via FakeSocket:
      Pair 1: hwts.tv_sec=1, precise_origin=2 s  → establishes base
      Pair 2: hwts.tv_sec=2, precise_origin=3 s  → delta=1 s > 500 ms threshold

    SyncStateMachine computes the master-time delta between consecutive pairs.
    When the delta exceeds jump_future_threshold_ns, GptpEngine sets
    is_time_jump_future=true and is_correct=false in the committed snapshot.

    Assertions:
      - Scenario exits with code 0.
      - time_jump_future_detected: true.
      - is_not_correct: true (is_correct=false when a jump is present).
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "gptp.time_jump_detection"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0.

        A non-zero exit indicates a failure during engine construction,
        Initialize(), or the frame-injection/polling phase.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_time_jump_future_detected(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_time_jump_future must be true after two pairs with a 1 s master delta.

        SyncStateMachine tracks the master-time progression between consecutive
        FollowUp frames.  A forward jump > 500 ms must set the flag.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="time_jump_future_detected", value="true"
        )

    def test_is_correct_cleared_on_jump(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_correct must be false whenever a time jump is active.

        The is_correct flag aggregates all anomaly conditions.  A detected
        time jump must result in is_correct=false to signal that the published
        gPTP time should not be used by consumers.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_not_correct", value="true")
