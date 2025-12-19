/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "AbortMock.h"

namespace score {
namespace time {
namespace common {

void logFatalAndAbort(const char* msg) noexcept {
    mock::AbortMock abortMock;
    abortMock.logFatalAndAbort(msg);
}

}  // namespace common
}  // namespace core
}  // namespace score
