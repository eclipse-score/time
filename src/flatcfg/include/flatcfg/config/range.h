/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_CONFIG_RANGE_H
#define FLATCFG_CONFIG_RANGE_H

namespace flatcfg
{
namespace config
{

/// @brief Type which acts as a sentinel for all config iterators.
struct Sentinel {};

/// @brief Helper instance of sentinel type.
// RULECHECKER_comment(2, 1, check_single_use_pod_variable, "Constexpr instance provided for utility.", true)
// coverity[autosar_cpp14_a0_1_1_violation:FALSE]
static constexpr Sentinel sentinel {};

}  // namespace config
}  // namespace flatcfg

#endif  // FLATCFG_CONFIG_RANGE_H
