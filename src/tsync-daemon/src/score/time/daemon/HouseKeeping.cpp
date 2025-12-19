/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/daemon/HouseKeeping.h"

#include <iostream>

using std::raise;
using std::signal;

#ifdef TSYNC_UT_BUILD
// This is required to get the std signal APIs to make use of our Signal Mock
#include "SignalMockDefines.h"
#endif

namespace score {
namespace time {
namespace daemon {

// set by signal handler, checked by worker
volatile std::sig_atomic_t HouseKeeping::exit_flag_ = 0;

} // namespace daemon
} // namespace time
} // namespace score

using score::time::daemon::HouseKeeping;

namespace {
    // coverity[autosar_cpp14_m3_4_1_violation] The scope of this symbol is defined as intended.
    std::sig_atomic_t signal_in_progress = 0;
}

// coverity[autosar_cpp14_m3_4_1_violation] The scope of this symbol is defined as intended.
void SignalHandler(int signal) {
    if ((signal == SIGINT) || (signal == SIGTERM)) {
        if (signal_in_progress) {
            (void)raise(signal);
        }
        signal_in_progress = 1;
        // This will end the run-loop in the worker instance
        HouseKeeping::exit_flag_ = 1;
    } else {
        // should never end up here
    }
}

namespace score {
namespace time {
namespace daemon {

void HouseKeeping::InitSignals() /* noexcept */ {
    InitFlags();

    (void)signal(SIGINT, SignalHandler);
    (void)signal(SIGTERM, SignalHandler);
}

void HouseKeeping::InitFlags() {
    exit_flag_ = 0;
    signal_in_progress = 0;
}

}  // namespace daemon
}  // namespace time
}  // namespace score
