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
#ifndef SCORE_TIME_SYSTEM_TIME_SYSTEM_CLOCK_MOCK_H
#define SCORE_TIME_SYSTEM_TIME_SYSTEM_CLOCK_MOCK_H

#include "score/time/system_time/system_clock_iface.h"
#include "score/time/system_time/system_clock.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the system-clock domain.
///
/// Implements @c SystemClockIface so it can be injected via
/// @c test_utils::ScopedClockOverride<std::chrono::system_clock> in unit tests.
///
/// Usage:
/// @code
///   auto mock = std::make_shared<SystemClockMock>();
///   test_utils::ScopedClockOverride<std::chrono::system_clock> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class SystemClockMock : public SystemClockIface
{
  public:
    SystemClockMock()                                    = default;
    ~SystemClockMock() noexcept override                 = default;
    SystemClockMock(const SystemClockMock&)              = delete;
    SystemClockMock& operator=(const SystemClockMock&)   = delete;
    SystemClockMock(SystemClockMock&&)                   = delete;
    SystemClockMock& operator=(SystemClockMock&&)        = delete;

    MOCK_METHOD((ClockSnapshot<std::chrono::system_clock::time_point, NoStatus>),
                Now,
                (),
                (const, noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYSTEM_TIME_SYSTEM_CLOCK_MOCK_H
