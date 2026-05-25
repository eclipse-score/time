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
#ifndef SCORE_TIME_CLOCK_SCOPED_CLOCK_OVERRIDE_H
#define SCORE_TIME_CLOCK_SCOPED_CLOCK_OVERRIDE_H

#include "score/time/clock/clock.h"
#include "score/time/clock/clock_traits.h"

#include <memory>

namespace score
{
namespace time
{
namespace test_utils
{

/// @brief RAII guard that installs a test-double backend for @c Clock<Tag>.
///
/// On construction calls @c Clock<Tag>::OverrideForTest(); on destruction calls
/// @c Clock<Tag>::ResetOverride(). Move-only; nesting two guards for the same
/// @c Tag is a logic error caught by assertion.
///
/// Use this when the system under test calls @c GetInstance() itself:
/// @code
///     auto mock = std::make_shared<MyMock>();
///     ScopedClockOverride<VehicleTime> guard{mock};
///     MySvc svc{};  // svc calls GetInstance() internally
///     EXPECT_CALL(*mock, Now())...;
///     svc.DoSomething();
/// @endcode
///
/// For constructor injection without a global override, use @c ClockTestFactory<Tag>.
template <typename Tag>
class ScopedClockOverride final
{
    using Backend = typename ClockTraits<Tag>::Backend;

  public:
    /// @brief Installs @p impl as the test backend; cleared on destruction.
    explicit ScopedClockOverride(const std::shared_ptr<Backend>& impl) noexcept : active_{true}
    {
        Clock<Tag>::OverrideForTest(impl);
    }

    /// @brief Removes the override if this guard is still active.
    ~ScopedClockOverride() noexcept
    {
        if (active_)
        {
            Clock<Tag>::ResetOverride();
        }
    }

    ScopedClockOverride(const ScopedClockOverride&)            = delete;
    ScopedClockOverride& operator=(const ScopedClockOverride&) = delete;

    /// @brief Transfers ownership; source guard is disarmed.
    ScopedClockOverride(ScopedClockOverride&& other) noexcept : active_{other.active_}
    {
        other.active_ = false;
    }

    ScopedClockOverride& operator=(ScopedClockOverride&&) = delete;

  private:
    bool active_;
};

}  // namespace test_utils
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SCOPED_CLOCK_OVERRIDE_H
