/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_PROGRAM_INFO_H
#define FLATCFG_UTILS_PROGRAM_INFO_H

#include "score/result/result.h"
#include "score/span.hpp"

#include <cstddef>

namespace flatcfg
{
namespace utils
{

/// @brief Class containing helper functions to obtain runtime information
///        about the currently running program.
class ProgramInfo
{
public:
    /// @brief Get the absolute null-terminated path to the executable of the
    ///        current process.
    /// @returns The size of the path written to the buffer (not including the
    ///          null terminator), or an error if the path cannot be found, if
    ///          the output buffer is empty, the buffer is not large enough to
    ///          hold the null-terminated path, or calling an OS function
    ///          results in an error.
    /// @warning The path written to the output buffer may not exist.
    static score::Result<std::size_t>
    executablePath(score::cpp::span<char> outputBuffer) noexcept;
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_PROGRAM_INFO_H
