/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_MATCHER_OPERATORS_H_
#define SCORE_TIME_MATCHER_OPERATORS_H_


#include <algorithm>
#include <cstddef>

#include "score/span.hpp"

namespace score {
namespace cpp {

inline bool operator==(const span<const std::byte>& v1, const span<const std::byte>& v2) {
    return (std::equal(std::begin(v1), std::end(v1), std::begin(v2), std::end(v2)));
}

inline bool operator!=(const span<const std::byte>& v1, const span<const std::byte>& v2) {
    return (!(v1 == v2));
}

}  // namespace cpp
}  // namespace score

#endif  // SCORE_TIME_MATCHER_OPERATORS_H_
