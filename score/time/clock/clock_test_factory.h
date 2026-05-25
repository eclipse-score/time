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
#ifndef SCORE_TIME_CLOCK_CLOCK_TEST_FACTORY_H
#define SCORE_TIME_CLOCK_CLOCK_TEST_FACTORY_H

#include "score/time/clock/clock.h"
#include "score/time/clock/clock_traits.h"

#include <memory>

namespace score
{
namespace time
{
namespace test_utils
{

/// @brief Test-only factory — constructs a @c Clock<Tag> wrapping an explicit backend.
///
/// Use for constructor injection in unit tests: supply a mock backend directly
/// without touching the process-wide singleton and without leaving any global
/// override active after the call returns.
///
/// @code
///   auto mock  = std::make_shared<HplsClockMock>();
///   auto clock = ClockTestFactory<HplsTime>::Make(mock);
///   TimeoutValidator v{clock, threshold};
///   EXPECT_CALL(*mock, Now())...;
/// @endcode
///
/// For the case where the system under test calls @c GetInstance() internally,
/// use @c ScopedClockOverride<Tag> instead.
template <typename Tag>
class ClockTestFactory
{
    using Backend = typename ClockTraits<Tag>::Backend;

  public:
    /// @brief Returns a @c Clock<Tag> whose backend is @p backend.
    [[nodiscard]] static Clock<Tag> Make(const std::shared_ptr<Backend>& backend) noexcept
    {
        return Clock<Tag>{backend};  // friend access to private ctor
    }

    ClockTestFactory()                                   = delete;
    ~ClockTestFactory()                                  = delete;
    ClockTestFactory(const ClockTestFactory&)            = delete;
    ClockTestFactory& operator=(const ClockTestFactory&) = delete;
    ClockTestFactory(ClockTestFactory&&)                 = delete;
    ClockTestFactory& operator=(ClockTestFactory&&)      = delete;
};

}  // namespace test_utils
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_CLOCK_TEST_FACTORY_H
