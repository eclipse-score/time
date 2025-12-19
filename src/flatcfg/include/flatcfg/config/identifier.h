/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_HANDLE_H
#define FLATCFG_CONFIG_HANDLE_H

#include "version.h"

#include <flatcfg/fs/_os.h>
#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace flatcfg
{

    // forward declaration so that we can friend it in Identifier
    class FlatCfg;

namespace config
{

/// @brief Unique identifier for a configuration.
class Identifier
{
    // friend FlatCfg so that it can access the complete path
    // RULECHECKER_comment(1, 1, check_friend_declaration, "Necessary for desired functionality.", true)
    friend class flatcfg::FlatCfg;

public:
    /// @brief Construct an identifier from a path to a configuration file, and
    ///        the expected version it should meet.
    /// @details The path is expected to match the following regex:
    ///   (.*/)?                       parent dirs
    ///   [a-z]+                       function cluster
    ///   (_[a-z0-9](_?[a-z0-9]+)*)?   process (leading underscore)
    ///   __.+                         software cluster (leading double underscore)
    ///   _flatcfg\.bin                suffix
    static score::Result<Identifier>
    FromPathAndVersion(std::string_view filePath, Version version) noexcept;

    /// @brief Copy (and move) constructible.
    Identifier(const Identifier&) = default;

    /// @brief Copy (and move) assignable.
    Identifier&
    operator=(const Identifier&) = default;

    /// @brief Get the configuration's function cluster.
    /// @note  Will always be non-empty.
    std::string_view
    functionCluster() const noexcept;

    /// @brief Get the configuration's software cluster.
    /// @note  Will always be non-empty.
    std::string_view
    softwareCluster() const noexcept;

    /// @brief Get the configuration's optional process identifier.
    /// @note  Will always be non-empty if not std::nullopt.
    std::optional<std::string_view>
    process() const noexcept;

    /// @brief Get the version the configuration is expected to meet.
    Version
    version() const noexcept;

    /// @brief Equality comparison.
    friend bool
    operator==(const Identifier& lhs, const Identifier& rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(const Identifier& lhs, const Identifier& rhs) noexcept;

private:
    /// @brief Construct an identifier from a path to a configuration file, and
    ///        the expected version it should meet.
    /// @details The path is expected to match the following regex:
    ///   (.*/)?                       parent dirs
    ///   [a-z]+                       function cluster
    ///   (_[a-z0-9](_?[a-z0-9]+)*)?   process (leading underscore)
    ///   __.+                         software cluster (leading double underscore)
    ///   _flatcfg\.bin                suffix
    /// @note  If an error occurs, it is written to resOut, and the instance is considered invalid.
    Identifier(std::string_view filePath, Version version,
               score::ResultBlank *resOut) noexcept;

    /// @brief Get view of the underlying path.
    std::string_view
    path() const noexcept;

    /// @brief Statically sized buffer to hold the configuration file path.
    std::array<char, fs::_os::MAX_PATH_LENGTH + 1U> m_pathBuffer {};

    /// @brief Size of complete path from start of path buffer, not including
    ///        null-terminator.
    std::size_t m_pathSize {};

    /// @brief Offset of function cluster from start of path buffer.
    std::ptrdiff_t m_fcPos {};

    /// @brief Size of function cluster from offset.
    std::size_t m_fcSize {};

    /// @brief Offset of software cluster from start of path buffer.
    std::ptrdiff_t m_swclPos {};

    /// @brief Size of software cluster from offset.
    std::size_t m_swclSize {};

    /// @brief Offset of optional process identifier from start of path buffer.
    std::optional<std::ptrdiff_t> m_procPosOpt {};

    /// @brief Size of optional process identifier from offset.
    std::optional<std::size_t> m_procSizeOpt {};

    /// @brief Version the found file is expected to meet.
    Version m_version {};
};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_HANDLE_H
