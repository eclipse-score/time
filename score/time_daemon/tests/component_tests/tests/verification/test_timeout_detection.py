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


class TestVerificationTimeoutDetection(TimeDaemonCitScenario):
    """Verifies TimeoutValidator flag-set/clear behavior inside SvtVerificationMachine.

    PtpTimeInfo frames are injected directly to drive the TimeoutValidator through
    its state transitions without relying on async PTP hardware:

      1. First frame (seq=1)      → is_timeout=false  (reception_time_ initialized)
      2. Wait 3.6 s (> 3.3 s threshold)
      3. Same frame (seq=1 again) → is_timeout=true   (no new frame, elapsed > threshold)
      4. New frame (seq=2)        → is_timeout=false  (reception_time_ reset, flag cleared)

    This provides integration coverage for the timeout path that the TimeoutValidator
    unit tests exercise only with a mock clock.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "verification.timeout_detection"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — all four phases completed.

        A non-zero exit indicates an exception in the verifier construction,
        Init(), or one of the OnMessage() calls.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_first_frame_does_not_trigger_timeout(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_timeout must be false immediately after the first frame.

        TimeoutValidator records reception_time_ on the first frame and resets
        is_timeout — it must never fire on the very first message.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="first_frame_not_timeout", value="true"
        )

    def test_timeout_detected_after_threshold(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_timeout must be true when the same frame is repeated after 3.3 s.

        Repeating the same sequence_id means TimeoutValidator sees no new frame.
        Once elapsed time exceeds the 3 300 000 000 ns threshold, is_timeout=true
        must be set and propagated through the pipeline.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="timeout_detected", value="true")

    def test_timeout_cleared_on_new_frame(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_timeout must be false after a frame with a new sequence_id arrives.

        A new sequence_id signals a fresh PTP frame; TimeoutValidator must reset
        reception_time_ and clear is_timeout, confirming the recovery path works.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="timeout_cleared_on_new_frame", value="true"
        )
