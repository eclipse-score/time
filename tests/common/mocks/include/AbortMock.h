/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_MOCK_ABORTMOCK_H
#define SCORE_TIME_MOCK_ABORTMOCK_H

#include <gmock/gmock.h>

namespace score {
namespace time {
namespace mock {

class AbortMock
{
public:
    MOCK_METHOD1(logFatalAndAbort, void(const char*));
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<::testing::NiceMock<AbortMock>> abort_mock;

} // namespace mock
} // namespace time
} // namespace score

#endif  // SCORE_TIME_MOCK_ABORTMOCK_H
