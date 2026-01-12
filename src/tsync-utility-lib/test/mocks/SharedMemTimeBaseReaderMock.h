/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_SHAREDMEMTIMEBASEREADERMOCK_H_
#define SCORE_TIME_SHAREDMEMTIMEBASEREADERMOCK_H_

#include <gmock/gmock.h>

#include "score/time/utility/TsyncConfigTypes.h"
#include "score/time/utility/ITimeBaseAccessor.h"
#include "score/time/utility/ITimeBaseReader.h"

namespace score {
namespace time {

class TsyncConsumerConfig;
class TsyncProviderConfig;

class SharedMemTimeBaseReaderMock : public ITimeBaseAccessor, public ITimeBaseReader {
   public:
    explicit SharedMemTimeBaseReaderMock(std::string_view /*name*/, uint32_t /* max_size */) noexcept {
        // Little hack to help with the TSyncWorker UTs.
        // The only purpose of this is to set the global sync status.
        // This can always be overwritten by individual tests.
        ON_CALL(*this, Read(testing::An<SynchronizationStatus&>()))
            .WillByDefault(
                testing::DoAll(testing::SetArgReferee<0>(SynchronizationStatus::kSynchronized), testing::Return(true)));

        // config injection
        reader_mock_cfg_.consumer_config.time_slave_config.is_valid = true;
        ON_CALL(*this, Read(testing::An<TsyncTimeDomainConfig&>()))
            .WillByDefault(testing::DoAll(testing::SetArgReferee<0>(reader_mock_cfg_), testing::Return(true)));

        ON_CALL(*this, GetAccessor()).WillByDefault(testing::ReturnRef(*this));

        // Have most boolean methods return success by default
        ON_CALL(*this, Read(testing::An<std::uint8_t&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<std::uint16_t&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<std::uint32_t&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<std::uint64_t&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<float&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<double&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<std::string&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<std::string_view&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<TimestampWithStatus&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<VirtualLocalTime&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<UserDataView&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<TsyncConsumerConfig&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Read(testing::An<TsyncProviderConfig&>())).WillByDefault(testing::Return(true));
        ON_CALL(*this, Skip(testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*this, SetPosition(testing::_)).WillByDefault(testing::Return(true));
        ON_CALL(*this, GetPosition()).WillByDefault(testing::Return(48));
        ON_CALL(*this, GetMaxSize()).WillByDefault(testing::Return(4096));
        ON_CALL(*this, GetState()).WillByDefault(testing::Return(ITimeBaseAccessor::State::Open));
    }
    ~SharedMemTimeBaseReaderMock() noexcept {
        Close();
    }
    SharedMemTimeBaseReaderMock() = delete;
    SharedMemTimeBaseReaderMock(const SharedMemTimeBaseReaderMock&) = delete;
    SharedMemTimeBaseReaderMock& operator=(const SharedMemTimeBaseReaderMock&) = delete;

    // ITimeBaseAccessor
    MOCK_METHOD0(Open, void());
    MOCK_METHOD0(Close, void());
    MOCK_CONST_METHOD0(GetName, std::string_view());
    MOCK_CONST_METHOD0(GetState, State());

    // ITimeBaseReader
    MOCK_METHOD0(GetAccessor, ITimeBaseAccessor&());

    MOCK_METHOD1(Read, bool(std::uint8_t&));
    MOCK_METHOD1(Read, bool(std::uint16_t&));
    MOCK_METHOD1(Read, bool(std::uint32_t&));
    MOCK_METHOD1(Read, bool(std::uint64_t&));
    MOCK_METHOD1(Read, bool(std::int8_t&));
    MOCK_METHOD1(Read, bool(std::int16_t&));
    MOCK_METHOD1(Read, bool(std::int32_t&));
    MOCK_METHOD1(Read, bool(std::int64_t&));
    MOCK_METHOD1(Read, bool(float&));
    MOCK_METHOD1(Read, bool(double&));
    MOCK_METHOD1(Read, bool(std::string&));
    MOCK_METHOD1(Read, bool(std::string_view&));
    MOCK_METHOD2(Read, bool(const char**, std::size_t));
    MOCK_METHOD1(Read, bool(TimestampWithStatus&));
    MOCK_METHOD1(Read, bool(VirtualLocalTime&));
    MOCK_METHOD1(Read, bool(UserDataView&));
    MOCK_METHOD1(Read, bool(SynchronizationStatus&));
    MOCK_METHOD1(Read, bool(TsyncTimeDomainConfig&));
    MOCK_METHOD1(Read, bool(TsyncConsumerConfig&));
    MOCK_METHOD1(Read, bool(TsyncProviderConfig&));
    MOCK_METHOD1(Skip, bool(std::size_t));
    MOCK_METHOD1(SetPosition, bool(std::size_t));
    MOCK_CONST_METHOD0(GetPosition, std::size_t());
    MOCK_CONST_METHOD0(GetMaxSize, std::size_t());
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());

   private:
    TsyncTimeDomainConfig reader_mock_cfg_;
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_SHAREDMEMTIMEBASEREADERMOCK_H_
