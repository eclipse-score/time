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
#ifndef SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_MOCK_H
#define SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_MOCK_H

#include "score/time/hirs_time/src/hirs_clock_backend.h"
#include "score/time/hirs_time/src/hirs_clock.h"
#include "score/time/clock/src/scoped_clock_override.h"
#include "score/time/clock/src/clock_test_factory.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the HIRS clock domain.
///
/// Implements @c HirsClockBackend so it can be injected via
/// @c test_utils::ScopedClockOverride<HirsTime> or @c test_utils::ClockTestFactory<HirsTime> in unit tests.
///
/// Constructor injection (preferred — no global state):
/// @code
///   auto mock  = std::make_shared<HirsClockBackendMock>();
///   auto clock = test_utils::ClockTestFactory<HirsTime>::Make(mock);
///   MyComponent component{clock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
///
/// Scope-bound override (when SUT calls @c GetInstance() internally):
/// @code
///   auto mock = std::make_shared<HirsClockBackendMock>();
///   test_utils::ScopedClockOverride<HirsTime> guard{mock};
///   MySvc svc{};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class HirsClockBackendMock : public HirsClockBackend
{
  public:
    HirsClockBackendMock()                                    = default;
    ~HirsClockBackendMock() noexcept override                 = default;
    HirsClockBackendMock(const HirsClockBackendMock&)                = delete;
    HirsClockBackendMock& operator=(const HirsClockBackendMock&)     = delete;
    HirsClockBackendMock(HirsClockBackendMock&&)                     = delete;
    HirsClockBackendMock& operator=(HirsClockBackendMock&&)          = delete;

    MOCK_METHOD((ClockSnapshot<HirsTime::Timepoint, NoStatus>),
                Now,
                (),
                (const, noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIRS_TIME_SRC_HIRS_CLOCK_BACKEND_MOCK_H
