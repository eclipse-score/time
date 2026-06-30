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
#ifndef SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_HIGH_RES_STEADY_CLOCK_BACKEND_MOCK_H
#define SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_HIGH_RES_STEADY_CLOCK_BACKEND_MOCK_H

#include "score/time/high_res_steady_time/src/high_res_steady_clock_backend.h"
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"
#include "score/time/clock/src/scoped_clock_override.h"
#include "score/time/clock/src/clock_test_factory.h"

#include <gmock/gmock.h>

namespace score
{
namespace time
{

/// @brief GMock test double for the HIRS clock domain.
///
/// Implements @c HighResSteadyClockBackend so it can be injected via
/// @c test_utils::ScopedClockOverride<HighResSteadyTime> or @c test_utils::ClockTestFactory<HighResSteadyTime> in unit tests.
///
/// Constructor injection (preferred — no global state):
/// @code
///   auto mock  = std::make_shared<HighResSteadyClockBackendMock>();
///   auto clock = test_utils::ClockTestFactory<HighResSteadyTime>::Make(mock);
///   MyComponent component{clock};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
///
/// Scope-bound override (when SUT calls @c GetInstance() internally):
/// @code
///   auto mock = std::make_shared<HighResSteadyClockBackendMock>();
///   test_utils::ScopedClockOverride<HighResSteadyTime> guard{mock};
///   MySvc svc{};
///   EXPECT_CALL(*mock, Now()).WillOnce(Return(...));
/// @endcode
class HighResSteadyClockBackendMock : public HighResSteadyClockBackend
{
  public:
    HighResSteadyClockBackendMock()                                    = default;
    ~HighResSteadyClockBackendMock() noexcept override                 = default;
    HighResSteadyClockBackendMock(const HighResSteadyClockBackendMock&)                = delete;
    HighResSteadyClockBackendMock& operator=(const HighResSteadyClockBackendMock&)     = delete;
    HighResSteadyClockBackendMock(HighResSteadyClockBackendMock&&)                     = delete;
    HighResSteadyClockBackendMock& operator=(HighResSteadyClockBackendMock&&)          = delete;

    MOCK_METHOD((ClockSnapshot<HighResSteadyTime::Timepoint, NoStatus>),
                Now,
                (),
                (const, noexcept, override));
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_HIGH_RES_STEADY_TIME_SRC_HIGH_RES_STEADY_CLOCK_BACKEND_MOCK_H
