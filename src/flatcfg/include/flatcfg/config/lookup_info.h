/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_LOOKUP_INFO_H
#define FLATCFG_CONFIG_LOOKUP_INFO_H

#include "version.h"

#include <flatcfg/fs/_os.h>
#include <flatcfg/_export.h>

#include "score/result/result.h"

#include <array>
#include <cstdint>
#include <optional>

namespace flatcfg
{
namespace config
{

/// @brief Class to hold information necessary to determine if a found configuration
///        is relevant or not.
class LookupInfo
{
public:
    /// @brief Create an instance of LookupInfo from values.
    /// @details The values are expected to match the following regexes:
    ///          [a-z]+                 function cluster
    ///          [a-z0-9](_?[a-z0-9]+)* process
    ///          The version can have any value.
    static score::Result<LookupInfo>
    FromValues(std::string_view functionCluster,
               std::optional<std::string_view> process,
               Version version) noexcept;

    /// @brief Copy (and move) constructible.
    LookupInfo(const LookupInfo&) = default;

    /// @brief Copy (and move) assignable.
    LookupInfo&
    operator=(const LookupInfo&) = default;

    /// @brief Get the relevant function cluster for a configuration.
    std::string_view
    functionCluster() const noexcept;

    /// @brief Get the optional relevant process for a configuration.
    std::optional<std::string_view>
    process() const noexcept;

    /// @brief Get the expected version a configuration should meet.
    Version
    version() const noexcept;

    /// @brief Check if this info is relevant for a configuration.
    bool
    isRelevantFor(std::string_view functionCluster,
                  std::optional<std::string_view> process) const noexcept;

private:
    /// @brief Create an instance of LookupInfo from values.
    /// @details The values are expected to match the following regexes:
    ///          [a-z]+                 function cluster
    ///          [a-z0-9](_?[a-z0-9]+)* process
    ///          The version can have any value.
    LookupInfo(std::string_view functionCluster,
               std::optional<std::string_view> process,
               Version version, score::ResultBlank *resOut) noexcept;

    /// @brief Statically sized buffer to hold lookup info values.
    std::array<char, fs::_os::MAX_NAME_LENGTH> m_buffer {};

    /// @brief Size of function cluster from start of buffer.
    std::size_t m_fcSize {};

    /// @brief Size of optional process from end of function cluster + 1.
    std::optional<std::size_t> m_procSizeOpt {};

    /// @brief Version the found file is expected to have.
    Version m_version {};
};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_LOOKUP_INFO_H
