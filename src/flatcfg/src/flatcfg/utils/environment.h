/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_ENVIRONMENT_H
#define FLATCFG_UTILS_ENVIRONMENT_H

#include "score/span.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace flatcfg
{
namespace utils
{

/// @brief Wrapper class to make it clear that functions are not member
///        functions of whichever other class they're called in.
class Environment
{
public:
    /// @brief Reads an environment variable's contents into a span.
    /// @details If the environment variable is not set, or the size of the
    ///            output buffer exceeds PTRDIFF_MAX or is 0, nullopt is returned.
    ///          Otherwise, the contents of the environment variable is copied until
    ///            the span is filled or the null-terminator is read. In both cases, the
    ///            last character written is a null-terminator.
    ///          If the span was filled before reading a null-terminator, the size of
    ///            the string written to the span (not including the null-terminator) is
    ///            returned as a negative value, otherwise the positive value is returned.
    /// @warning The envName MUST be null-terminated.
    /// @pre  The output buffer span does not point to the environment variable value.
    /// @post Any remaining bytes after the null-terminator in the output buffer are zeroed.
    static std::optional<std::ptrdiff_t>
    get(std::string_view envName, score::cpp::span<char> outputBuffer) noexcept;

    /// @brief Calls get(...) with PROCESSIDENTIFIER as the environment variable.
    static std::optional<std::ptrdiff_t>
    processIdentifier(score::cpp::span<char> outputBuffer) noexcept;

    /// @brief Calls get(...) with ECUCFG_ENV_VAR_ROOTFOLDER as the environment variable.
    static std::optional<std::ptrdiff_t>
    ecuCfgRootFolder(score::cpp::span<char> outputBuffer) noexcept;
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_ENVIRONMENT_H
