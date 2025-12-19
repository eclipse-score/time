/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <cstdlib>
#include <iostream>
//!!#include "score/mw/log/logging.h"

namespace score {
namespace time {
namespace common {

[[noreturn]] void logFatalAndAbort(const char* msg) noexcept {
    //!!score::mw::LogFatal() << msg << std::endl;
    std::cerr << "FATAL: " << msg << std::endl;
    std::abort();   
}

}  // namespace commmon
}  // namespace time
}  // namespace score
