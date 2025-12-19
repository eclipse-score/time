/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_PATH_COMPONENTS_H
#define FLATCFG_UTILS_PATH_COMPONENTS_H

#include "score/result/result.h"

#include <optional>
#include <string_view>

namespace flatcfg
{
namespace utils
{

/// @brief Helper type to hold views to components of a config file path.
struct PathComponents
{
    std::string_view filePath;
    std::string_view fileName;
    std::string_view functionCluster;
    std::string_view softwareCluster;
    std::optional<std::string_view> process;

    /// @brief Parse a config file path into its components.
    /// @return The components, or an error if the path was not formatted correctly.
    /// @pre Path matches the regex:
    ///   (.*/)?                       parent dirs
    ///   [a-z]+                       function cluster
    ///   (_[a-z0-9](_?[a-z0-9]+)*)?   process (leading underscore)
    ///   __.+                         software cluster (leading double underscore)
    ///   _flatcfg\.bin                suffix
    /// @post All std::string_views will be non-empty and substrings of input path.
    static score::Result<PathComponents>
    FromPath(std::string_view path) noexcept;
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_PATH_COMPONENTS_H
