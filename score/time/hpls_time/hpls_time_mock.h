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
#ifndef SCORE_TIME_HPLS_TIME_HPLS_TIME_MOCK_H
#define SCORE_TIME_HPLS_TIME_HPLS_TIME_MOCK_H

#include "score/time/hpls_time/hpls_clock_iface.h"
#include "score/time/hpls_time/hpls_clock.h"
#include "score/time/clock/clock_override_guard.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the HPLS clock domain.
///
/// Implements @c HplsClockIface so it can be injected via
/// @c ClockOverrideGuard<HplsTime> in unit tests.
///
/// Usage:
/// @code
///   auto mock = std::make_shared<HplsTimeMock>();
///   ClockOverrideGuard<HplsTime> guard{mock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class HplsTimeMock : public HplsClockIface
{
  public:
    HplsTimeMock()                                   = default;
    ~HplsTimeMock() noexcept override                = default;
    HplsTimeMock(const HplsTimeMock&)                = delete;
    HplsTimeMock& operator=(const HplsTimeMock&)     = delete;
    HplsTimeMock(HplsTimeMock&&)                     = delete;
    HplsTimeMock& operator=(HplsTimeMock&&)          = delete;

    MOCK_METHOD((ClockSnapshot<HplsTime::Timepoint, NoStatus>),
                Now,
                (),
                (const, noexcept, override));
};

/// @brief Creates an @c HplsClock backed by @p mock for isolated unit tests.
///
/// The process-wide override is installed only transiently — for a single
/// @c GetInstance() call — and then immediately removed.  The returned
/// @c HplsClock still holds a @c shared_ptr to @p mock via its internal
/// @c impl_ member, so @p mock controls every subsequent @c Now() call on
/// that clock without leaving any process-wide state behind.
///
/// Usage:
/// @code
///   auto mock  = std::make_shared<HplsTimeMock>();
///   auto clock = MakeHplsClockFrom(mock);   // no override active after this line
///   TimeoutValidator validator{clock, timeout};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
[[nodiscard]] inline HplsClock MakeHplsClockFrom(std::shared_ptr<HplsTimeMock> mock)
{
    ClockOverrideGuard<HplsTime> guard{mock};
    return HplsClock::GetInstance();
}  // guard destroyed here — process-wide override cleared

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HPLS_TIME_HPLS_TIME_MOCK_H
