/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SHAREDMEMTIMEBASEWRITERMOCK_H_
#define SCORE_TIME_SHAREDMEMTIMEBASEWRITERMOCK_H_

#include <gmock/gmock.h>

#include <functional>

#include "score/span.hpp"

#include "score/time/utility/ITimeBaseAccessor.h"
#include "score/time/utility/ITimeBaseWriter.h"

namespace score {
namespace time {

class TsyncConsumerConfig;
class TsyncProviderConfig;
class SharedMemTimeBaseWriterMock : public ITimeBaseAccessor, public ITimeBaseWriter {
public:
    SharedMemTimeBaseWriterMock(std::string_view, std::size_t, bool) {
        ON_CALL(*this, GetAccessor()).WillByDefault(testing::ReturnRef(*this));
        ON_CALL(*this, Write(testing::An<uint32_t>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<std::string_view>())).WillByDefault(testing::Return(true));

        ON_CALL(*this, Write(testing::An<const SynchronizationStatus&>())).WillByDefault(testing::Return(true));

        // Have most boolean methods return success by default
        ON_CALL(*this, Write(testing::An<std::uint8_t>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<std::uint16_t>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<std::uint32_t>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<std::uint64_t>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<float>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<double>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const std::string&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<score::cpp::span<std::byte>>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const TimestampWithStatus&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const VirtualLocalTime&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<UserData>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<UserDataView>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const TsyncTimeDomainConfig&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const TsyncConsumerConfig&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Write(testing::An<const TsyncProviderConfig&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, WriteDefaults()).WillByDefault(testing::Return(true));
        ON_CALL(*this, Skip(testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*this, SetPosition(testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*this, GetPosition()).WillByDefault(testing::Return(48));
        ON_CALL(*this, GetMaxSize()).WillByDefault(testing::Return(4096));
        ON_CALL(*this, GetState()).WillByDefault(testing::Return(ITimeBaseAccessor::State::Open));
    };

    ~SharedMemTimeBaseWriterMock() {
        Close();
    };

    SharedMemTimeBaseWriterMock() = delete;
    SharedMemTimeBaseWriterMock(const SharedMemTimeBaseWriterMock&) = delete;
    SharedMemTimeBaseWriterMock& operator=(const SharedMemTimeBaseWriterMock&) = delete;

    // ITimeBaseAccessor
    MOCK_METHOD0(Open, void());
    MOCK_METHOD0(Close, void());
    MOCK_CONST_METHOD0(GetName, std::string_view());
    MOCK_CONST_METHOD0(GetState, State());

    // ITimeBaseWriter
    MOCK_METHOD0(GetAccessor, ITimeBaseAccessor&());
    MOCK_METHOD1(Write, bool(std::uint8_t));
    MOCK_METHOD1(Write, bool(std::uint16_t));
    MOCK_METHOD1(Write, bool(std::uint32_t));
    MOCK_METHOD1(Write, bool(std::uint64_t));
    MOCK_METHOD1(Write, bool(std::int8_t));
    MOCK_METHOD1(Write, bool(std::int16_t));
    MOCK_METHOD1(Write, bool(std::int32_t));
    MOCK_METHOD1(Write, bool(std::int64_t));
    MOCK_METHOD1(Write, bool(float));
    MOCK_METHOD1(Write, bool(double));
    MOCK_METHOD1(Write, bool(const std::string&));
    MOCK_METHOD1(Write, bool(std::string_view));
    MOCK_METHOD1(Write, bool(score::cpp::span<std::byte>));
    MOCK_METHOD1(Write, bool(const TimestampWithStatus&));
    MOCK_METHOD1(Write, bool(const VirtualLocalTime&));
    MOCK_METHOD1(Write, bool(UserData));
    MOCK_METHOD1(Write, bool(UserDataView));
    MOCK_METHOD1(Write, bool(const SynchronizationStatus&));
    MOCK_METHOD1(Write, bool(const TsyncTimeDomainConfig&));
    MOCK_METHOD1(Write, bool(const TsyncConsumerConfig&));
    MOCK_METHOD1(Write, bool(const TsyncProviderConfig&));
    MOCK_METHOD2(Write, bool(const char*, std::size_t));
    MOCK_METHOD0(WriteDefaults, bool());
    MOCK_METHOD1(Skip, bool(std::size_t));
    MOCK_METHOD1(SetPosition, bool(std::size_t));
    MOCK_CONST_METHOD0(GetPosition, std::size_t());
    MOCK_CONST_METHOD0(GetMaxSize, std::size_t());
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SHAREDMEMTIMEBASEWRITERMOCK_H_
