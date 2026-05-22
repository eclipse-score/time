/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_TIME_DAEMON_SRC_IPC_CORE_TEST_TYPES_H
#define SCORE_TIME_DAEMON_SRC_IPC_CORE_TEST_TYPES_H

#include <cstdint>

#include "score/time_daemon/src/ipc/data_converter.h"

namespace score
{
namespace td
{
namespace test
{

struct FakeTimeInfo
{
    uint64_t ptp_assumed_time;
    uint64_t local_time;
};

bool operator==(const FakeTimeInfo& first, const FakeTimeInfo& second) noexcept
{
    const bool same_local = (first.local_time == second.local_time);
    const bool same_ptp = (first.ptp_assumed_time == second.ptp_assumed_time);

    return (same_local && same_ptp);
}

bool operator!=(const FakeTimeInfo& first, const FakeTimeInfo& second) noexcept
{
    return !(first == second);
}

struct FakeTimeInfoIpc
{
    uint64_t ptp_assumed_time;
    uint64_t local_time;

    void CreateFrom(const FakeTimeInfo& info);
};

void FakeTimeInfoIpc::CreateFrom(const FakeTimeInfo& info)
{
    ptp_assumed_time = info.ptp_assumed_time;
    local_time = info.local_time;
}

bool operator==(const FakeTimeInfoIpc& first, const FakeTimeInfoIpc& second) noexcept
{
    const bool same_local = (first.local_time == second.local_time);
    const bool same_ptp = (first.ptp_assumed_time == second.ptp_assumed_time);

    return (same_local && same_ptp);
}

bool operator!=(const FakeTimeInfoIpc& first, const FakeTimeInfoIpc& second) noexcept
{
    return !(first == second);
}

bool operator==(const FakeTimeInfo& data, const FakeTimeInfoIpc& ipcdata) noexcept
{
    const bool same_local = (data.local_time == ipcdata.local_time);
    const bool same_ptp = (data.ptp_assumed_time == ipcdata.ptp_assumed_time);

    return (same_local && same_ptp);
}

bool operator==(const FakeTimeInfoIpc& ipcdata, const FakeTimeInfo& data) noexcept
{
    return (data == ipcdata);
}

bool operator!=(const FakeTimeInfo& data, const FakeTimeInfoIpc& ipcdata) noexcept
{
    return !(data == ipcdata);
}

bool operator!=(const FakeTimeInfoIpc& ipcdata, const FakeTimeInfo& data) noexcept
{
    return !(ipcdata == data);
}

}  // namespace test

/**
 * \brief DataConverter specialization for test types
 */
template <>
struct DataConverter<test::FakeTimeInfo, test::FakeTimeInfoIpc>
{
    static test::FakeTimeInfoIpc Convert(const test::FakeTimeInfo& src)
    {
        test::FakeTimeInfoIpc dst{};
        dst.CreateFrom(src);
        return dst;
    }
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_IPC_CORE_TEST_TYPES_H
