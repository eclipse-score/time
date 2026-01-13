/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_DAEMON_HOUSEKEEPING_H_
#define SCORE_TIME_DAEMON_HOUSEKEEPING_H_

#include <csignal>

namespace score {
namespace time {
namespace daemon {

class HouseKeeping {
public:
    static void Init() noexcept;

    // This flag will be set when an exit signal was received
    static volatile std::sig_atomic_t exit_flag_;

    HouseKeeping() = delete;
};

}  // namespace daemon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_DAEMON_HOUSEKEEPING_H_
