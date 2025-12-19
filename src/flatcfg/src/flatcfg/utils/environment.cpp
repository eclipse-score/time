/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "environment.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>

using score::cpp::span;

namespace flatcfg
{
namespace utils
{

std::optional<std::ptrdiff_t>
Environment::get(std::string_view envName, span<char> outputBuffer) noexcept
{
    // unsupported buffer size -> nullopt
    constexpr unsigned long long umax_ptrdiff = static_cast<unsigned long long>(std::numeric_limits<std::ptrdiff_t>::max());
    if (outputBuffer.empty() || outputBuffer.size() > umax_ptrdiff)
    {
        return std::nullopt;
    }

    // nullptr -> nullopt
    char *envPtr = std::getenv(envName.data());
    if (envPtr == nullptr)
    {
        return std::nullopt;
    }

    // copy environment variable into buffer
    static_cast<void>(std::strncpy(outputBuffer.data(), envPtr, outputBuffer.size()));

    // find null-terminator
    auto it = std::find(outputBuffer.begin(), outputBuffer.end(), '\0');
    if (it == outputBuffer.end())
    {
        // no null-terminator written, environment variable value was too large
        outputBuffer.back() = '\0';
        return -static_cast<std::ptrdiff_t>(outputBuffer.size() - 1U);
    }
    else
    {
        return std::distance(outputBuffer.begin(), it);
    }
}

std::optional<std::ptrdiff_t>
Environment::processIdentifier(span<char> outputBuffer) noexcept
{
    return get("PROCESSIDENTIFIER", outputBuffer);
}

std::optional<std::ptrdiff_t>
Environment::ecuCfgRootFolder(span<char> outputBuffer) noexcept
{
    return get("ECUCFG_ENV_VAR_ROOTFOLDER", outputBuffer);
}

}  // namespace utils
}  // namespace flatcfg
