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
#ifndef SCORE_TIMEDAEMON_CODE_APPLICATION_TIMEDAEMON_H
#define SCORE_TIMEDAEMON_CODE_APPLICATION_TIMEDAEMON_H

#include "score/time_daemon/src/application/timebase_handler.h"

#include "src/lifecycle_client_lib/include/application.h"

namespace score
{
namespace td
{

class TimeDaemon final : public score::mw::lifecycle::Application
{
  public:
    explicit TimeDaemon();
    ~TimeDaemon() noexcept override = default;

    TimeDaemon(TimeDaemon&&) noexcept = delete;
    TimeDaemon(const TimeDaemon&) noexcept = delete;
    TimeDaemon& operator=(TimeDaemon&&) & noexcept = delete;
    TimeDaemon& operator=(const TimeDaemon&) & noexcept = delete;

    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& context) override;
    std::int32_t Run(const score::cpp::stop_token& token) override;

  private:
    std::unique_ptr<TimebaseHandler> svt_timebase_handler_;
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_APPLICATION_TIMEDAEMON_H
