/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_LOG_COMMON_H
#define FLATCFG_LOG_COMMON_H

#include <cstdint>

namespace flatcfg
{
namespace log
{

/// @brief Equivalent to score::mw::log::LogLevel.
enum class LogLevel : std::uint8_t
{
    /// @brief No logging.
    kOff,

    /// @brief Fatal error, not recoverable.
    kFatal,

    /// @brief Error with impact to correct functionality.
    kError,

    /// @brief Warning if correct behaviour cannot be ensured.
    kWarn,

    /// @brief Informational, providing high level understanding.
    kInfo,

    /// @brief Detailed information for programmers.
    kDebug,

    /// @brief Extra-verbose debug messages (highest grade of information).
    kVerbose
};

/// @brief Mapping of log level label to string.
// coverity[autosar_cpp14_m3_4_1_violation] This has to be defined here by design.
constexpr const char *LogLevelNames[] = {
    "kOff",
    "kFatal",
    "kError",
    "kWarn",
    "kInfo",
    "kDebug",
    "kVerbose"
};

}  // namespace log
}  // namespace flatcfg

#endif  // FLATCFG_LOG_COMMON_H
