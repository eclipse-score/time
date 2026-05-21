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
#ifndef SCORE_TIMEDAEMON_CODE_PTP_MACHINE_CORE_PTP_ENGINE_MOCK_H
#define SCORE_TIMEDAEMON_CODE_PTP_MACHINE_CORE_PTP_ENGINE_MOCK_H

#include "score/time_daemon/src/common/data_types/ptp_time_info.h"

#include <memory>

#include <gmock/gmock.h>

namespace score
{
namespace td
{
namespace testing
{

class PTPEngineMockInterface
{
  public:
    virtual ~PTPEngineMockInterface() = default;
    virtual bool Initialize() = 0;
    virtual bool Deinitialize() = 0;
    virtual bool ReadPTPSnapshot(PtpTimeInfo& info) = 0;
};

class PTPEngineMock : public PTPEngineMockInterface
{
  public:
    PTPEngineMock() {}

    MOCK_METHOD(bool, Initialize, (), (override));
    MOCK_METHOD(bool, Deinitialize, (), (override));
    MOCK_METHOD(bool, ReadPTPSnapshot, (PtpTimeInfo&), (override));
};

class PTPEngineMockProvider
{
  public:
    static PTPEngineMockProvider& GetInstance()
    {
        static PTPEngineMockProvider provider;
        return provider;
    }

    std::shared_ptr<PTPEngineMock> GetMock()
    {
        return obj_;
    }

    void CreateMock()
    {
        obj_ = std::make_shared<PTPEngineMock>();
    }

    void DestroyMock()
    {
        obj_.reset();
    }

  private:
    std::shared_ptr<PTPEngineMock> obj_;
};

class FakePTPEngine
{
  public:
    FakePTPEngine()
    {
        PTPEngineMockProvider::GetInstance().CreateMock();
    }

    ~FakePTPEngine()
    {
        PTPEngineMockProvider::GetInstance().DestroyMock();
    }

    bool Initialize()
    {
        return PTPEngineMockProvider::GetInstance().GetMock()->Initialize();
    }

    bool Deinitialize()
    {
        return PTPEngineMockProvider::GetInstance().GetMock()->Deinitialize();
    }

    bool ReadPTPSnapshot(PtpTimeInfo& info)
    {
        return PTPEngineMockProvider::GetInstance().GetMock()->ReadPTPSnapshot(info);
    }
};

}  // namespace testing
}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_PTP_MACHINE_CORE_PTP_ENGINE_MOCK_H
