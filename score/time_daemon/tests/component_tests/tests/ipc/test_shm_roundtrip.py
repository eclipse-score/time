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


class TestIpcShmRoundtrip(TimeDaemonCitScenario):
    """Verifies the end-to-end SHM IPC pipeline: SvtPublisher writes a
    PtpTimeInfo, SvtReceiver reads back a TimeBaseSnapshot, and all key
    fields survive the DataConverter serialisation round-trip.

    The scenario exercises the production IPC path in-process:
      1. SvtPublisher.Init() initialises the SharedMemoryHandler.
      2. SvtPublisher.OnMessage() converts PtpTimeInfo → TimeBaseSnapshot
         via DataConverter and writes it to shared memory.
      3. SvtReceiver.Init() opens the same shared memory region.
      4. SvtReceiver.Receive() reads the TimeBaseSnapshot via the seqlock.
      5. Field values are compared against the originals.

    Input data:
      ptp_assumed_time = 1 000 000 000 ns (1 s)
      status           = {is_synchronized=true, is_correct=true}
      sync_fup_data.sequence_id = 42

    Assertions:
      - Scenario exits with code 0.
      - Receive() returns a value (SHM write/read path is operational).
      - ptp_assumed_time is preserved through the conversion.
      - is_synchronized and is_correct are preserved.
      - sync_fup_data.sequence_id is preserved.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "ipc.shm_roundtrip"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {}

    def test_scenario_exits_successfully(self, results: ScenarioResult):
        """Scenario binary must exit with code 0.

        A non-zero exit indicates failure during Init(), OnMessage(), or
        an unexpected exception in the IPC pipeline.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert not results.hang

    def test_read_succeeded(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """Receive() must return a value after OnMessage() writes to SHM.

        SharedMemoryHandler uses a seqlock; a successful write followed by a
        read on the same path must always yield data.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="read_succeeded", value="true")

    def test_ptp_time_preserved(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """ptp_assumed_time (1 s = 1 000 000 000 ns) must survive the roundtrip.

        DataConverter maps PtpTimeInfo.ptp_assumed_time.count() to
        TimeBaseSnapshot.ptp_assumed_time (uint64).  The nanosecond value
        must be bit-identical after serialisation.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="ptp_time_preserved", value="true")

    def test_status_preserved(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """is_synchronized and is_correct must both be true after the roundtrip.

        DataConverter maps PtpStatus fields to TimeBaseStatus fields.
        A correctly functioning converter must preserve all boolean flags.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="status_preserved", value="true")

    def test_seq_id_preserved(
        self, results: ScenarioResult, logs_info_level: LogContainer
    ):
        """sync_fup_data.sequence_id (42) must survive the roundtrip.

        DataConverter maps SyncFupData.sequence_id to SyncFupSnapshot.sequence_id.
        The uint16 value must be identical after serialisation.
        """
        assert results.return_code == ResultCode.SUCCESS
        assert logs_info_level.contains_log(field="seq_id_preserved", value="true")
