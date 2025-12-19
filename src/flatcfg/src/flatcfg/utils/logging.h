/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_LOGGING_H
#define FLATCFG_UTILS_LOGGING_H

#include "pretty_function.h"

#include "flatcfg/log/logger.h"

namespace flatcfg
{
namespace utils
{
namespace _impl
{

/// @brief Get static logger instance.
/// @note  Inline so that we don't need to mock it in unit tests.
inline log::Logger&
getLogger() noexcept
{
    // RULECHECKER_comment(1, 1, check_static_object_dynamic_initialization, "Static variable is a function local so will not cause issues.", true)
    static log::Logger logger { "flatcfg", log::LogLevel::kInfo };
    return logger;
}

}  // namespace _impl
}  // namespace utils
}  // namespace flatcfg

#define FLATCFG_LOG_(level)                          \
    (flatcfg::utils::_impl::getLogger().Log##level() \
        << '[' << FLATCFG_PRETTY_FUNCTION << "] ")

#define FLATCFG_LOG_FATAL()   FLATCFG_LOG_(Fatal)
#define FLATCFG_LOG_ERROR()   FLATCFG_LOG_(Error)
#define FLATCFG_LOG_WARN()    FLATCFG_LOG_(Warn)
#define FLATCFG_LOG_INFO()    FLATCFG_LOG_(Info)
#define FLATCFG_LOG_DEBUG()   FLATCFG_LOG_(Debug)
#define FLATCFG_LOG_VERBOSE() FLATCFG_LOG_(Verbose)

#endif  // FLATCFG_UTILS_LOGGING_H
