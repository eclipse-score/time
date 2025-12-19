/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_LOG_LOGGER_H
#define FLATCFG_LOG_LOGGER_H

#include "common.h"
#include "log_stream.h"

#include <string_view>

#include <array>
#include <cstddef>

namespace flatcfg
{
namespace log
{

/// @brief Class which will create loggers of various levels.
/// @details Any stream from a function whose log level is lower than the log
///          level the logger instance was constructed with will behave as if
///          the log level is kOff.
class Logger
{
public:
    /// @brief Construct logger from a name and log level.
    Logger(std::string_view name, LogLevel logLevel) noexcept;

    /// @brief Create log stream which logs at the fatal level.
    LogStream LogFatal() noexcept;

    /// @brief Create log stream which logs at the error level.
    LogStream LogError() noexcept;

    /// @brief Create log stream which logs at the warn level.
    LogStream LogWarn() noexcept;

    /// @brief Create log stream which logs at the info level.
    LogStream LogInfo() noexcept;

    /// @brief Create log stream which logs at the debug level.
    LogStream LogDebug() noexcept;

    /// @brief Create log stream which logs at the verbose level.
    LogStream LogVerbose() noexcept;

private:
    /// @brief Create log stream which logs at the given level.
    LogStream LogAt(LogLevel logLevel) noexcept;

    /// @brief Level beneath which we will not log.
    LogLevel m_logLevel {};

    /// @brief Name of instance, not null-terminated.
    std::array<char, 32> m_nameBuf {};

    /// @brief Size of name.
    std::size_t m_nameSize {};
};

}  // namespace log
}  // namespace flatcfg

#endif  // FLATCFG_LOG_LOGGER_H
