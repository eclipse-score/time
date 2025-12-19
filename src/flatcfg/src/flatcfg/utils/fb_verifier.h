/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_FB_VERIFIER_H
#define FLATCFG_UTILS_FB_VERIFIER_H

#include "score/result/result.h"

#include <cstdint>
#include <string_view>

namespace flatcfg
{
namespace utils
{

/// @brief Wrapper class around flatbuffers verification helpers.
class FbVerifier
{
public:
    /// @brief Verify a flatbuffers binary's function cluster info.
    static score::ResultBlank
    verifyBinary(std::string_view binaryData,
                 std::string_view functionCluster,
                 std::int32_t versionMajor, std::int32_t versionMinor) noexcept;
};

}  // namespace utils
}  // namespace flatcfg

#endif  // FLATCFG_UTILS_FB_VERIFIER_H
