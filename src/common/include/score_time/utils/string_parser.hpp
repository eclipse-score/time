/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#pragma once

#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

namespace score_time::utils
{

    /**
     * @brief Parse integer from string without exceptions (C++17 std::from_chars)
     *
     * @tparam T Integer type (int, long, int64_t, uint32_t, etc.)
     * @param str Input string to parse
     * @param out Output parameter to store parsed value
     * @return true if parsing succeeded, false otherwise
     *
     * @note This function never throws exceptions
     */
    template <typename T>
    [[nodiscard]] bool ParseInteger(std::string_view str, T &out) noexcept
    {
        if (str.empty())
        {
            return false;
        }

        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
        return ec == std::errc{} && ptr == str.data() + str.size();
    }

    /**
     * @brief Parse double from string without exceptions
     *
     * @param str Input string to parse
     * @param out Output parameter to store parsed value
     * @return true if parsing succeeded, false otherwise
     *
     * @note Uses strtod with careful error handling since std::from_chars
     *       for floating point may not be available in all C++17 implementations
     * @note Uses fixed-size buffer to avoid dynamic allocation and exceptions.
     *       Maximum input length is 127 characters.
     */
    [[nodiscard]] inline bool ParseDouble(std::string_view str, double &out) noexcept
    {
        if (str.empty())
        {
            return false;
        }

        // Use fixed-size buffer to avoid std::string allocation (no exceptions)
        constexpr std::size_t kMaxDoubleStrLen = 127;
        if (str.size() > kMaxDoubleStrLen)
        {
            return false;  // String too long
        }

        // Copy to null-terminated buffer for strtod
        char buffer[kMaxDoubleStrLen + 1];
        std::memcpy(buffer, str.data(), str.size());
        buffer[str.size()] = '\0';

        char *end = nullptr;
        errno = 0;
        const double val = std::strtod(buffer, &end);

        // Check for errors:
        // - errno == ERANGE: overflow or underflow
        // - end != expected: not all characters consumed
        if (errno == ERANGE || end != buffer + str.size())
        {
            return false;
        }

        out = val;
        return true;
    }

    /**
     * @brief Parse integer from std::string (convenience overload)
     */
    template <typename T>
    [[nodiscard]] bool ParseInteger(const std::string &str, T &out) noexcept
    {
        return ParseInteger(std::string_view(str), out);
    }

    /**
     * @brief Parse double from std::string (convenience overload)
     */
    [[nodiscard]] inline bool ParseDouble(const std::string &str, double &out)
    {
        return ParseDouble(std::string_view(str), out);
    }

} // namespace score_time::utils
