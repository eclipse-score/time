/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_MOCK_SIGNAL_H_
#define SCORE_TIME_MOCK_SIGNAL_H_

#include <gmock/gmock.h>
#include <iostream>
#include <csignal>

class SignalMock {
public:
    MOCK_METHOD2(signalMock, int(int, void(*)(int)));
    MOCK_METHOD1(raiseMock, int(int));
};

static SignalMock* g_mock_signal = nullptr;

int MockRaise(int signum) {
    return g_mock_signal->raiseMock(signum);
}

int MockSignal(int signum, void(*handler)(int)) {   
    return g_mock_signal->signalMock(signum, handler);
}

#include "SignalMockDefines.h"

#endif  // SCORE_TIME_MOCK_SIGNAL_H_
