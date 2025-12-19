/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_COMMON_ABORT_H_
#define SCORE_TIME_COMMON_ABORT_H_

namespace score {
namespace time {
namespace common {

[[noreturn]] void logFatalAndAbort(const char* msg) noexcept;

}  // namespace commmon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_COMMON_ABORT_H_
