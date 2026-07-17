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


class TestGptpSyncAcquisition(TimeSlaveGptpCitScenario):
    """Verifies that GptpEngine transitions to is_synchronized=true after receiving
    one Sync+FollowUp frame pair injected via a software-only FakeSocket.

    Frame injection path (no real hardware):
      1. GptpEngine is constructed with FakeSocket + FakeIdentity.
      2. Initialize() starts the RxThread and PdelayThread.
      3. One Sync frame (seq=1, hwts.tv_sec=1) followed by a FollowUp
         (seq=1, precise_origin = 2 s) is pushed into FakeSocket.
      4. RxThread dequeues both, routes them through SyncStateMachine, and
         commits the snapshot via UpdateSnapshot().
      5. FinalizeSnapshot() + ReadPTPSnapshot() polling confirms the result.

    Assertions:
      - Scenario exits with code 0.
      - is_synchronized=true after processing the pair.
      - is_timeout=false (reception_time_ was just recorded).
      - ptp_assumed_time > 0 (timestamps were propagated).
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "gptp.sync_acquisition"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0.

        A non-zero exit indicates a failure in Initialize(), or an unhandled
        exception during frame injection or snapshot polling.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_sync_acquired(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_synchronized must become true after one valid Sync+FollowUp pair.

        SyncStateMachine correlates the Sync hardware timestamp with the precise
        origin from FollowUp and produces a SyncResult that sets is_synchronized.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="sync_acquired", value="true")

    def test_no_timeout_on_fresh_sync(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_timeout must be false immediately after a successful sync.

        FinalizeSnapshot() must not set is_timeout when the elapsed time since
        the last Sync is well within the configured sync_timeout_ms window.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="is_not_timeout", value="true")

    def test_ptp_time_is_positive(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """ptp_assumed_time must be positive after sync.

        The precise origin timestamp from the FollowUp frame must propagate
        through SyncStateMachine and into the committed GptpIpcData snapshot.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_time_positive", value="true")
