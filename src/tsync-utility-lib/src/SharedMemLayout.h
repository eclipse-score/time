/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_SHAREDMEMLAYOUT_H_
#define SCORE_TIME_UTILITY_SHAREDMEMLAYOUT_H_

#include <cstddef>

namespace score {
namespace time {

constexpr std::size_t kSharedMemPageSize = 4096u;
constexpr std::size_t kSharedMemTimebaseDataOffset = 0u;
constexpr std::size_t kSharedMemTimebaseConfigOffset = 2048u;

constexpr std::size_t kSharedMemTimebaseDomainConfigOffset = kSharedMemTimebaseConfigOffset;
constexpr std::size_t kSharedMemTimebaseConsumerConfigOffset = kSharedMemTimebaseDomainConfigOffset + 512u;
constexpr std::size_t kSharedMemTimebaseProviderConfigOffset = kSharedMemTimebaseConsumerConfigOffset + 512u;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_SHAREDMEMLAYOUT_H_
