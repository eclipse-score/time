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


class TestDaemonLifecycle(TimeDaemonCitScenario):
    """Verifies that the TimeDaemon subsystem stack can be constructed, initialized,
    and stopped cleanly without requiring IPC or external hardware.

    The scenario exercises all four component factory functions:
      - CreateGPTPStubMachine      (PTP data source)
      - CreateSvtVerificationMachine (time verification pipeline)
      - CreateSvtPublisher         (IPC publisher)
      - CreatePtpControlFlowDivider (control flow routing)

    It also validates that SvtHandler::Initialize() wires the full MessageBroker
    pub-sub topology, and that Stop() called from the kIdle state is a safe no-op —
    confirming the graceful-shutdown path before any async worker has been started.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "daemon.lifecycle"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0 — all lifecycle phases completed.

        A non-zero exit code indicates that at least one factory function threw
        an exception, Initialize() failed, or Stop() raised an error.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_lifecycle_initialization_succeeds(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm that Initialize() completed without error.

        Verifies that CreateSvtTimebase() constructs all four subsystems and
        SvtHandler::Initialize() successfully wires the three MessageBroker
        pub-sub topics (in_ptp_data, raw_ptp_data, validated_ptp_data).
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(
            field="lifecycle_initialize_ok", value="true"
        )

    def test_lifecycle_completes(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """TRACING_INFO must confirm that Stop() returned cleanly from kIdle state.

        Stop() must be safe to call before any subsystem has been started (i.e.
        before RunOnce() has transitioned the handler out of kIdle).  This test
        guards against regressions where Stop() incorrectly tries to stop machines
        that were never started.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="lifecycle_complete", value="true")
