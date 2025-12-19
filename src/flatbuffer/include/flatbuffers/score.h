/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATBUFFERS_SCORE_H_
#define FLATBUFFERS_SCORE_H_

/// @note This file is intentionally not available through flatbuffers.h to
///       maintain separation between standard flatbuffers and our extensions.

#include <cstdint>

namespace flatbuffers {
namespace score {

/// @brief Base class for functional cluster info to be used via CRTP to generate
///        derived classes.
/// @note  This is the closest we can get to an abstract base class when our
///        functions are static constexpr.
template <typename T>
struct FcBase {

  /// @brief Major version of functional cluster.
  static constexpr std::int32_t versionMajor() noexcept {
    return T::_versionMajor();
  }

  /// @brief Minor version of functional cluster.
  static constexpr std::int32_t versionMinor() noexcept {
    return T::_versionMinor();
  }

  /// @brief Name of functional cluster (identical to derived type's name).
  static constexpr const char * name() noexcept {
    return T::_name();
  }
};

}  // namespace score
}  // namespace flatbuffers

#endif  // FLATBUFFERS_SCORE_H_
