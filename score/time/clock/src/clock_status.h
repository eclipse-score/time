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

#ifndef SCORE_TIME_CLOCK_SRC_CLOCK_STATUS_H
#define SCORE_TIME_CLOCK_SRC_CLOCK_STATUS_H

#include <sstream>
#include <cstdint>
#include <type_traits>

#include <score/assert.hpp>

namespace score
{
namespace time
{

/// @brief Encapsulates the synchronisation-quality status of a timebase as a set of bit flags.
///
/// This class is the successor to @c TimeBaseStatus. It is used by timebases that express their
/// quality state as a set of named bit-positions (e.g. @c VehicleTime::StatusFlag). Timebases
/// with no quality concept (e.g. HirsTime) use @c NoStatus instead.
///
/// @tparam FlagEnumT  Scoped enum type whose enumerators are **bit positions** (not bitmasks).
///                    The underlying type must be an unsigned integer type.
template <typename FlagEnumT>
class ClockStatus final
{
    static_assert(std::is_enum<FlagEnumT>::value, "FlagEnumT must be an enum");
    using StatusFlagT = std::underlying_type_t<FlagEnumT>;

  public:
    /// @brief Constructs a @c ClockStatus with the given set of active flags.
    ///
    /// @param flag_list  List of flag bit-positions to set as active.
    ClockStatus(const std::initializer_list<FlagEnumT> flag_list) noexcept : status_flags_{}
    {
        for (const auto flag : flag_list)
        {
            AddStatusFlagTo(status_flags_, flag);
        }
    }

    ClockStatus() = default;
    ClockStatus(const ClockStatus&) = default;
    ClockStatus(ClockStatus&&) noexcept = default;
    ClockStatus& operator=(const ClockStatus&) = default;
    ClockStatus& operator=(ClockStatus&&) noexcept = default;
    ~ClockStatus() noexcept = default;

    /// @brief Returns @c true if the given flag bit-position is set in this status.
    ///
    /// @param flag  The bit-position to test.
    bool IsFlagActive(const FlagEnumT flag) const noexcept
    {
        const StatusFlagT flag_position{static_cast<StatusFlagT>(flag)};
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            (flag_position < (sizeof(StatusFlagT) * 8U)),
            "score::time: IsFlagActive() argument 'flag' exceeds possible size of status_flags_ type");
        return static_cast<bool>(status_flags_ & (static_cast<StatusFlagT>(1U << flag_position)));
    }

    /// @brief Returns @c true if any of the given flag bit-positions is set in this status.
    ///
    /// @param flag_list  The bit-positions to test.
    bool IsAnyOfFlagsActive(const std::initializer_list<FlagEnumT>& flag_list) const noexcept
    {
        bool is_any_flag_set{false};
        for (const auto flag : flag_list)
        {
            if (IsFlagActive(flag))
            {
                is_any_flag_set = true;
                break;
            }
        }
        return is_any_flag_set;
    }

    /// @brief Formats all active flags into an @c ostringstream for diagnostics.
    ///
    /// @note Requires a template specialization for the concrete @c FlagEnumT.
    std::ostringstream PrintTo() const;

    /// @brief Adds a flag bit-position to this status.
    ///
    /// @param flag  The bit-position to set.
    void AddFlag(const FlagEnumT flag) noexcept
    {
        AddStatusFlagTo(status_flags_, flag);
    }

    /// @brief Compares two @c ClockStatus objects for equality.
    friend bool operator==(const ClockStatus& lhs, const ClockStatus& rhs) noexcept
    {
        return lhs.status_flags_ == rhs.status_flags_;
    }

    /// @brief Returns the underlying raw flag bitmask.
    constexpr StatusFlagT ToUnderlying() const noexcept
    {
        return status_flags_;
    }

    /// @brief Replaces the internal flag bitmask with the given raw value.
    ///
    /// @param status_flags  New raw bitmask to store.
    void FromUnderlying(const StatusFlagT status_flags) noexcept
    {
        status_flags_ = status_flags;
    }

  private:
    /// @brief Adds the bit-position represented by @p flag into @p status_container.
    static void AddStatusFlagTo(StatusFlagT& status_container, const FlagEnumT flag) noexcept
    {
        const StatusFlagT flag_position{static_cast<StatusFlagT>(flag)};
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(
            (flag_position < (sizeof(status_container) * 8U)),
            "score::time: AddStatusFlagTo() argument 'flag' exceeds possible size of 'StatusFlagT& status_container'");
        const StatusFlagT temp{static_cast<StatusFlagT>(1U << flag_position)};
        status_container = static_cast<StatusFlagT>(status_container | temp);
    }

    /// @brief Raw storage for all active flag bits.
    StatusFlagT status_flags_{};
};

/// @brief Stream insertion operator — delegates to ClockStatus::PrintTo().
template <typename OutputStream, typename FlagEnumT>
auto operator<<(OutputStream& output_stream, const ClockStatus<FlagEnumT>& clock_status) -> OutputStream&
{
    output_stream << clock_status.PrintTo().str();
    return output_stream;
}

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_CLOCK_SRC_CLOCK_STATUS_H
