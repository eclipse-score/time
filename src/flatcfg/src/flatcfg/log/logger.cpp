/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "logger.h"

#include <algorithm>
#include <iterator>

namespace flatcfg
{
namespace log
{

Logger::Logger(std::string_view name, LogLevel logLevel) noexcept
    : m_logLevel(logLevel)
{
    // copy name, possibly truncating it
    if (name.size() > m_nameBuf.size())
    {
        name.remove_suffix(name.size() - m_nameBuf.size());
    }
    auto it = std::copy(name.begin(), name.end(), m_nameBuf.begin());
    m_nameSize = static_cast<std::size_t>(std::distance(m_nameBuf.begin(), it));
}

LogStream
Logger::LogFatal() noexcept
{
    return LogAt(LogLevel::kFatal);
}

LogStream
Logger::LogError() noexcept
{
    return LogAt(LogLevel::kError);
}

LogStream
Logger::LogWarn() noexcept
{
    return LogAt(LogLevel::kWarn);
}

LogStream
Logger::LogInfo() noexcept
{
    return LogAt(LogLevel::kInfo);
}

LogStream
Logger::LogDebug() noexcept
{
    return LogAt(LogLevel::kDebug);
}

LogStream
// coverity[autosar_cpp14_a7_5_2_violation:Intentional] False positive.
Logger::LogVerbose() noexcept
{
    return LogAt(LogLevel::kVerbose);
}

LogStream
Logger::LogAt(LogLevel logLevel) noexcept
{
    // figure out log level
    if (m_logLevel == LogLevel::kOff ||
        static_cast<int>(m_logLevel) < static_cast<int>(logLevel))
    {
        logLevel = LogLevel::kOff;
    }

    // log prelude
    LogStream stream { logLevel };
    std::string_view name { m_nameBuf.data(), m_nameSize };
    stream << "[" << name << "]["
           << LogLevelNames[static_cast<int>(logLevel)] << "] ";

    // return
    return stream;
}

}  // namespace log
}  // namespace flatcfg
