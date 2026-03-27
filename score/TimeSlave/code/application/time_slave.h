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
#ifndef SCORE_TIMESLAVE_CODE_APPLICATION_TIME_SLAVE_H
#define SCORE_TIMESLAVE_CODE_APPLICATION_TIME_SLAVE_H

#include "score/TimeSlave/code/gptp/gptp_engine.h"
#include "score/libTSClient/gptp_ipc_publisher.h"

#include "src/lifecycle_client_lib/include/application.h"

#include <memory>

namespace score
{
namespace ts
{

/**
 * @brief Standalone TimeSlave process that runs the gPTP engine
 *        and publishes time data to shared memory.
 *
 * TimeSlave is the gPTP protocol endpoint. It runs GptpEngine internally
 * (with RxThread + PdelayThread) and periodically writes PtpTimeInfo
 * to shared memory for consumption by TimeDaemon via ShmPTPEngine.
 */
class TimeSlave final : public score::mw::lifecycle::Application
{
  public:
    explicit TimeSlave();
    ~TimeSlave() noexcept override = default;

    TimeSlave(TimeSlave&&) noexcept = delete;
    TimeSlave(const TimeSlave&) noexcept = delete;
    TimeSlave& operator=(TimeSlave&&) & noexcept = delete;
    TimeSlave& operator=(const TimeSlave&) & noexcept = delete;

    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& context) override;
    std::int32_t Run(const score::cpp::stop_token& token) override;

  private:
    details::GptpEngineOptions              opts_;
    std::unique_ptr<details::GptpEngine>    engine_;
    details::GptpIpcPublisher               publisher_;
};

}  // namespace ts
}  // namespace score

#endif  // SCORE_TIMESLAVE_CODE_APPLICATION_TIME_SLAVE_H
