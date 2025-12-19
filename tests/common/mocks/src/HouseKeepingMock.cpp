/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "HouseKeepingMock.h"
#include <csignal>

namespace score {
namespace time {

void HouseKeeping::InitSignals() {
    // do nothing
}

volatile std::sig_atomic_t HouseKeeping::exit_flag_ = 0;

} // namespace time
} // namespace score
