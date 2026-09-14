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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H

#include "score/time_daemon/src/application/timebase_handler.h"

#include "score/mw/lifecycle/application.h"

namespace score::td
{

class TimeDaemon final : public score::mw::lifecycle::Application
{
  public:
    explicit TimeDaemon();
    ~TimeDaemon() noexcept override = default;

    TimeDaemon(TimeDaemon&&) noexcept = delete;
    TimeDaemon(const TimeDaemon&) noexcept = delete;
    auto operator=(TimeDaemon&&) & noexcept -> TimeDaemon& = delete;
    auto operator=(const TimeDaemon&) & noexcept -> TimeDaemon& = delete;

    auto Initialize(const score::mw::lifecycle::ApplicationContext& context) -> std::int32_t override;
    auto Run(const score::cpp::stop_token& token) -> std::int32_t override;

  private:
    std::unique_ptr<TimebaseHandler> svt_timebase_handler_;
};

}  // namespace score::td

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIME_DAEMON_H
