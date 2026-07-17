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


class TestGptpSyncTimeout(TimeSlaveGptpCitScenario):
    """Verifies that GptpEngine sets is_timeout=true and clears is_synchronized
    when no new Sync frame arrives within the configured reception window.

    The scenario uses sync_timeout_ms=100 (instead of the production 3300 ms)
    so the CIT completes in well under one second.

    Sequence:
      1. Achieve sync with one Sync+FollowUp pair → is_synchronized=true.
      2. Stop injecting frames; sleep 250 ms (> 100 ms timeout).
      3. Call FinalizeSnapshot() → checks wall-clock elapsed time and sets
         is_timeout=true, clears is_synchronized.

    Assertions:
      - Scenario exits with code 0.
      - Initially synchronized: true (sync was established before timeout wait).
      - timeout_detected: true (elapsed > sync_timeout_ms).
      - sync_cleared: true (is_synchronized=false after timeout).
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "gptp.sync_timeout"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0.

        A non-zero exit indicates a failure during engine construction,
        Initialize(), or the sync-acquisition polling phase.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_initially_synchronized(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Engine must report is_synchronized=true before the timeout elapses.

        Confirms that the first Sync+FollowUp pair was correctly processed;
        otherwise, timeout_detected could be a false positive.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="initially_synchronized", value="true"
        )

    def test_timeout_detected(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_timeout must be true after no Sync frames for sync_timeout_ms.

        FinalizeSnapshot() compares the current wall-clock time against the
        last recorded reception timestamp.  When the gap exceeds the threshold,
        is_timeout must be set.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="timeout_detected", value="true")

    def test_sync_cleared_on_timeout(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_synchronized must be false once timeout has been triggered.

        The timeout path in FinalizeSnapshot() must clear the synchronized flag
        to prevent the rest of the system from acting on stale time data.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="sync_cleared", value="true")
