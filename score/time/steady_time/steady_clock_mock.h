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
#ifndef SCORE_TIME_STEADY_TIME_STEADY_CLOCK_MOCK_H
#define SCORE_TIME_STEADY_TIME_STEADY_CLOCK_MOCK_H

#include "score/time/steady_time/steady_clock_iface.h"
#include "score/time/steady_time/steady_clock.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the steady-clock domain.
///
/// Implements @c SteadyClockIface so it can be injected via
/// @c test_utils::ScopedClockOverride<std::chrono::steady_clock> in unit tests.
///
/// Usage:
/// @code
///   auto mock = std::make_shared<SteadyClockMock>();
///   test_utils::ScopedClockOverride<std::chrono::steady_clock> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class SteadyClockMock : public SteadyClockIface
{
  public:
    SteadyClockMock()                                    = default;
    ~SteadyClockMock() noexcept override                 = default;
    SteadyClockMock(const SteadyClockMock&)              = delete;
    SteadyClockMock& operator=(const SteadyClockMock&)   = delete;
    SteadyClockMock(SteadyClockMock&&)                   = delete;
    SteadyClockMock& operator=(SteadyClockMock&&)        = delete;

    MOCK_METHOD((ClockSnapshot<std::chrono::steady_clock::time_point, NoStatus>),
                Now,
                (),
                (const, noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_STEADY_TIME_STEADY_CLOCK_MOCK_H
