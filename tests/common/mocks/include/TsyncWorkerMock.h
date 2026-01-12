/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_WORKER_MOCK_H_
#define SCORE_TIME_WORKER_MOCK_H_
#include <gmock/gmock.h>

namespace score {
namespace time {

// Mock class for score::time::TsyncWorker
class TsyncWorkerMock {
public:
    MOCK_METHOD0(Init, void());
    MOCK_METHOD0(Run, void());
    MOCK_METHOD0(ShutDown, void());
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<TsyncWorkerMock> worker_mock;

// Stub class for score::time::TsyncWorker
class TsyncWorker {
public:
    TsyncWorker() noexcept;
    ~TsyncWorker() noexcept = default;
    TsyncWorker(const TsyncWorker&) = delete;
    TsyncWorker& operator=(const TsyncWorker&) = delete;
    void Init(void) noexcept;
    void Run(void) noexcept;
    void ShutDown(void) noexcept;
};

TsyncWorker::TsyncWorker() noexcept {
}

void TsyncWorker::Init() noexcept {
    worker_mock->Init();
}

void TsyncWorker::Run() noexcept {
    worker_mock->Run();
}

void TsyncWorker::ShutDown() noexcept {
    worker_mock->ShutDown();
}

} /* namespace time */
} /* namespace score */

#endif /*SCORE_TIME_WORKER_MOCK_H_ */
