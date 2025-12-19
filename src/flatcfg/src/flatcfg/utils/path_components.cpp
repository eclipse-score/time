/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "path_components.h"
#include "logging.h"

#include "flatcfg/flatcfg_error.h"

#include <string_view>

using score::Result;

namespace
{

constexpr std::string_view PROC_NAME_CHARS { "abcdefghijklmnopqrstuvwxyz0123456789_", 37 };
// coverity[autosar_cpp14_m3_4_1_violation:Intentional] Defined here for better maintainability.
constexpr std::string_view FC_NAME_CHARS { PROC_NAME_CHARS.substr(0U, 26U) };
// coverity[autosar_cpp14_m3_4_1_violation:Intentional] Defined here for better maintainability.
constexpr std::string_view FLATCFG_BIN_SUFFIX { "_flatcfg.bin", 12U };

}  // namespace

namespace flatcfg
{
namespace utils
{

Result<PathComponents>
PathComponents::FromPath(std::string_view path) noexcept
{
    // ensure path is not empty
    if (path.empty())
    {
        FLATCFG_LOG_WARN() <<
            "path cannot be empty";
        return score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
    }

    // ensure the file name is not empty
    if (path.back() == '/')
    {
        FLATCFG_LOG_WARN() <<
            "the final name component cannot be empty in the path: " << path;
        return score::MakeUnexpected(FlatCfgErrorCode::kBadPathName);
    }

    PathComponents comps;
    comps.filePath = path;

    // path should still contain: (.*/)? [a-z]+ (_[a-z0-9](_?[a-z0-9]+)*)? __.+ _flatcfg\.bin
    // parse file name and save to path
    {
        auto lastSlashPos = path.find_last_of('/');
        if (lastSlashPos == std::string_view::npos)
        {
            comps.fileName = path;
        }
        else
        {
            // skip last slash
            ++lastSlashPos;
            // empty file name check done earlier
            comps.fileName = path.substr(lastSlashPos);
        }
        path = comps.fileName;
    }

    // path should still contain: [a-z]+ (_[a-z0-9](_?[a-z0-9]+)*)? __.+ _flatcfg\.bin
    // remove: _flatcfg\.bin
    {
        // coverity[autosar_cpp14_a4_7_1_violation:Intentional] Will not wrap due to program design.
        if (path.rfind(FLATCFG_BIN_SUFFIX) != (path.size() - FLATCFG_BIN_SUFFIX.size()))
        {
            FLATCFG_LOG_DEBUG() <<
                "path does not reference a configuration; name does not end in"
                " '_flatcfg.bin': " << comps.filePath;
            return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
        }
        path.remove_suffix(FLATCFG_BIN_SUFFIX.size());
    }

    // path should still contain: [a-z]+ (_[a-z0-9](_?[a-z0-9]+)*)? __.+
    // parse function cluster and remove: [a-z]+
    {
        auto firstUnderscorePos = path.find_first_of('_');
        if (firstUnderscorePos == std::string_view::npos)
        {
            FLATCFG_LOG_DEBUG() <<
                "path does not reference a configuration; name does not contain"
                " a '_': " << comps.filePath;
            return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
        }
        else if (firstUnderscorePos == 0U)
        {
            FLATCFG_LOG_DEBUG() <<
                "path does not reference a configuration; name begins with a"
                " '_' (no function cluster name): " << comps.filePath;
            return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
        }
        else
        {
            comps.functionCluster = path.substr(0U, firstUnderscorePos);
            for (char c : comps.functionCluster)
            {
                if (FC_NAME_CHARS.find(c) == std::string_view::npos)
                {
                    FLATCFG_LOG_DEBUG() <<
                        "path does not reference a configuration; function"
                        " cluster component does not match '[a-z]+': " <<
                        comps.filePath;
                    return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
                }
            }
            path.remove_prefix(comps.functionCluster.size());
        }
    }

    // path should still contain: (_[a-z0-9](_?[a-z0-9]+)*)? __.+
    // parse process and remove: (_[a-z0-9](_?[a-z0-9]+)*)?
    {
        auto firstDoubleUnderscorePos = path.find("__");
        if (firstDoubleUnderscorePos == std::string_view::npos)
        {
            FLATCFG_LOG_DEBUG() <<
                "path does not reference a configuration; name does not contain"
                " a '__': " << comps.fileName;
            return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
        }
        else if (firstDoubleUnderscorePos == 0U)
        {
            comps.process = std::nullopt;
        }
        else
        {
            // skip leading underscore
            path.remove_prefix(1U);
            --firstDoubleUnderscorePos;
            comps.process = path.substr(0U, firstDoubleUnderscorePos);
            for (char c : *comps.process)
            {
                if (PROC_NAME_CHARS.find(c) == std::string_view::npos)
                {
                    FLATCFG_LOG_DEBUG() <<
                        "path does not reference a configuration; process name"
                        " component does not match '(_[a-z0-9](_?[a-z0-9]+)*)?__': " <<
                        comps.filePath;
                    return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
                }
            }
            path.remove_prefix(comps.process->size());
        }
    }

    // path should still contain: __.+
    // parse software cluster and remove: __.+
    {
        // remove leading double underscore
        path.remove_prefix(2U);
        if (path.empty())
        {
            FLATCFG_LOG_DEBUG() <<
                "path does not reference a configuration; does not contain a"
                " software cluster name: " << comps.filePath;
            return score::MakeUnexpected(FlatCfgErrorCode::kBadArgument);
        }
        comps.softwareCluster = path;
    }
    
    // success
    FLATCFG_LOG_DEBUG() <<
        "successfully parsed components (functionClusterName=" <<
        comps.functionCluster << ", softwareClusterName=" << comps.softwareCluster <<
        ", processName=" << comps.process.value_or("{none}") << ") from"
        " path: " << comps.filePath;
    return comps;
}

}  // namespace utils
}  // namespace flatcfg
