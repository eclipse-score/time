/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "HouseKeeping.h"

namespace score {
namespace time {
namespace daemon {

void SignalHandler(int /*signal*/) {
    // This will end the run-loop in the worker instance
    HouseKeeping::exit_flag_ = 1;
}

void HouseKeeping::Init() noexcept {
    exit_flag_ = 0;
    (void)std::signal(SIGINT, SignalHandler);
    (void)std::signal(SIGTERM, SignalHandler);
}

// set by signal handler, checked by worker
volatile std::sig_atomic_t HouseKeeping::exit_flag_ = 0;

} // namespace daemon
} // namespace time
} // namespace score
