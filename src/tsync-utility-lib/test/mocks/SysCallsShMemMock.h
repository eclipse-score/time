/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SYSCALLSSHMEMMOCK_H_
#define SCORE_TIME_SYSCALLSSHMEMMOCK_H_

#include <gmock/gmock.h>
#include <semaphore.h>

#include <memory>

namespace score {
namespace time {

class SysCallsShMemMock {
public:
    SysCallsShMemMock() {
        ON_CALL(*this, OsShmOpen(::testing::_, ::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsShmUnlink(::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsFtruncate(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsMmap(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_)).WillByDefault(::testing::Return(nullptr));
        ON_CALL(*this, OsMunmap(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*this, OsClose(::testing::_)).WillByDefault(::testing::Return(0));
    }
    MOCK_METHOD3(OsShmOpen, int(const char*, int, mode_t));
    MOCK_METHOD1(OsShmUnlink, int(const char*));
    MOCK_METHOD2(OsFtruncate, int(int, off_t));
    MOCK_METHOD6(OsMmap, void*(void*, size_t, int, int, int, off_t));
    MOCK_METHOD2(OsMunmap, int(void*, size_t));
    MOCK_METHOD1(OsClose, int(int));

    ~SysCallsShMemMock() = default;
};

// Declare the mock object and initialize it in the test module
extern std::unique_ptr<::testing::NiceMock<SysCallsShMemMock>> shared_mem_mock;

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SYSCALLSSHMEMMOCK_H_
