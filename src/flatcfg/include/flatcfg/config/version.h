/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_VERSION_H
#define FLATCFG_CONFIG_VERSION_H

#include <flatcfg/_export.h>

#include <cstdint>

namespace flatcfg
{
namespace config
{

/// @brief Helper struct to combine version components so that they don't get
///        mixed up when passed between functions.
struct Version
{
    /// @brief Major version component.
    std::int32_t major {};

    /// @brief Minor version component.
    std::int32_t minor {};

    /// @brief Equality comparison.
    friend bool
    operator==(Version lhs, Version rhs) noexcept;

    /// @brief Inequality comparison.
    friend bool
    operator!=(Version lhs, Version rhs) noexcept;
};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_VERSION_H
