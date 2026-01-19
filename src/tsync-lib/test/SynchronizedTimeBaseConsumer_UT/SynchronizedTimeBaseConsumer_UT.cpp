/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>
#include <cstddef>
#include <mutex>
#include <string_view>
#include <thread>
#include <type_traits>

//#include "tsync-ptp-lib/tsync_ptp_lib_types.h"
#include "matcher_operators.h"
#include "ConsumerTimeBaseValidationNotificationMock.h"
#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseWriterMock.h"
#include "TimeBaseReaderFactoryMock.h"
#include "TimeBaseWriterFactoryMock.h"
#include "TsyncSharedUtilsMock.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncSharedUtils.h"

#define private public
// class under test
#include "SynchronizedTimeBaseCommon.h"
#include "score/time/synchronized_time_base_consumer.h"
#undef private

namespace score {
namespace time {
// defined in SynchronizedTimeBaseCommon.cpp
extern TsyncIdMappingsHandler mappings_handler;
}  // namespace time
}  // namespace score

using InstanceSpecifier = std::string_view;
using score::time::reader_factory_mock;
using score::time::shared_utils_mock;
using score::time::writer_factory_mock;
using namespace score::time;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;

class SynchronizedTimeBaseConsumerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();
        shared_utils_mock = std::make_unique<::testing::NiceMock<TsyncSharedUtilsMock>>();
        mappings_handler.Clear();
        ASSERT_TRUE(mappings_handler.AddDomainMapping(1U, "one"));
        ASSERT_TRUE(mappings_handler.AddDomainMapping(2U, "two"));
        ASSERT_TRUE(mappings_handler.AddConsumerToDomain(1U, "consumer1"));
        ASSERT_TRUE(mappings_handler.AddConsumerToDomain("two", "consumer2"));

        reader_factory_mock_return_real_reader = true;
        writer_factory_mock_return_real_writer = true;
        mappings_handler.CommitMappingsToSharedMemory();
        mappings_handler.DoSharedMemoryRead();
        ASSERT_FALSE(mappings_handler.IsEmpty());
        reader_factory_mock_return_real_reader = false;
        writer_factory_mock_return_real_writer = false;

        // install abort handler for our death tests
        std::signal(SIGABRT, &AbortHandler);
        // As we use singleton mock objecty, clear expectations after each test
        ::testing::Mock::AllowLeak(reader_factory_mock.get());
        ::testing::Mock::AllowLeak(writer_factory_mock.get());
        ::testing::Mock::AllowLeak(shared_utils_mock.get());
    }

    void TearDown() override {
        CleanUp();
        std::signal(SIGABRT, SIG_DFL);
    }

    static void CleanUp() {
        reader_factory_mock.reset();
        writer_factory_mock.reset();
        shared_utils_mock.reset();
    }

    static void AbortHandler(int /*signal*/) noexcept {
        CleanUp();
        std::exit(EXIT_CODE);
    }

    static const int32_t EXIT_CODE;
};

const int32_t SynchronizedTimeBaseConsumerFixture::EXIT_CODE = 1;

using SynchronizedTimeBaseCommonFixture = SynchronizedTimeBaseConsumerFixture;

namespace testing {
namespace synchronizedtimebaseconsumer_ut {

TEST_F(SynchronizedTimeBaseConsumerFixture, Ctor_Succeeds) {
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    // defaults are checked
    ASSERT_NE(stbc.time_base_reader_.get(), nullptr);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, Ctor_OnInvalidConsumerName_Aborts) {
    ASSERT_EXIT({ SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("invalid")); },
                ::testing::ExitedWithCode(EXIT_CODE), "couldn't find time domain for consumer");
}

TEST_F(SynchronizedTimeBaseConsumerFixture, Dtor_Succeeds) {
    SynchronizedTimeBaseConsumer *stbc = new SynchronizedTimeBaseConsumer(InstanceSpecifier("consumer2"));
    delete stbc;
}

TEST_F(SynchronizedTimeBaseConsumerFixture, MoveCtor_Succeeds) {
    SynchronizedTimeBaseConsumer stbc1(InstanceSpecifier("consumer1"));
    // for later comparision
    auto timeBaseReaderPtrComp = stbc1.time_base_reader_.get();
    SynchronizedTimeBaseConsumer stbc2(std::move(stbc1));

    ASSERT_EQ(stbc2.time_base_reader_.get(), timeBaseReaderPtrComp);
    ASSERT_EQ(stbc1.time_base_reader_, nullptr);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, MoveAssignment_Succeeds) {
    SynchronizedTimeBaseConsumer stbc1(InstanceSpecifier("consumer1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    SynchronizedTimeBaseConsumer stbc2(InstanceSpecifier("consumer2"));
    // stb1 != stc2
    ASSERT_NE(stbc1.time_base_reader_.get(), stbc2.time_base_reader_.get());

    // for later comparision
    auto timeBaseReaderPtrComp = stbc1.time_base_reader_.get();

    // values are moved form stbc1 to stbc2
    stbc2 = std::move(stbc1);

    // stb2 == stc1 after move
    ASSERT_EQ(stbc2.time_base_reader_.get(), timeBaseReaderPtrComp);

    ASSERT_EQ(stbc1.time_base_reader_, nullptr);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetCurrentTime_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());
    score::time::TimestampWithStatus ts = {SynchronizationStatus::kNotSynchronizedUntilStartup,
                                          std::chrono::nanoseconds(0), std::chrono::seconds(43)};
    const uint32_t ns_diff{100000U};
    VirtualLocalTime lt(ns_diff);

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Open()).Times(2);
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<score::time::TimestampWithStatus &>()))
        .WillRepeatedly(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<VirtualLocalTime &>())).WillRepeatedly(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);

    // return real reader for handling of timebase mappings
    // reader_factory_mock_return_real_reader = true;
    uint64_t ns_sum = (std::chrono::duration_cast<std::chrono::nanoseconds>(ts.seconds) + ts.nanoseconds).count();
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(ns_sum)));
    auto ts1 = stbc.GetCurrentTime();

    ns_sum += static_cast<uint64_t>(ns_diff);

    EXPECT_CALL(*reader_mock, Open()).Times(2);
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<score::time::TimestampWithStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<VirtualLocalTime &>())).WillOnce(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(ns_sum)));

    auto ts2 = stbc.GetCurrentTime();

    std::chrono::nanoseconds nsHelper(ns_diff);

    auto d_cmp =
        std::chrono::duration_cast<Timestamp::duration>(ts1.time_since_epoch() + std::chrono::nanoseconds(ns_diff));

    ASSERT_LE(d_cmp.count(), ts2.time_since_epoch().count());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetCurrentTime_WithMultipleConsumer_Succeeds) {
    // 1. Create consumers object
    std::string_view shmem_name("/one");
    std::string_view shmem_name2("/two");
    SynchronizedTimeBaseConsumer *stbc = new SynchronizedTimeBaseConsumer(InstanceSpecifier("consumer1"));
    SynchronizedTimeBaseConsumer *stbc2 = new SynchronizedTimeBaseConsumer(InstanceSpecifier("consumer2"));
    // 2. Set Up for function GetCurrentTime for consumer1
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc->time_base_reader_.get());
    score::time::TimestampWithStatus ts = {SynchronizationStatus::kNotSynchronizedUntilStartup,
                                          std::chrono::nanoseconds(0), std::chrono::seconds(43)};
    const uint32_t ns_diff{100000U};
    VirtualLocalTime lt(ns_diff);

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Open()).Times(2);
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<score::time::TimestampWithStatus &>()))
        .WillRepeatedly(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<VirtualLocalTime &>())).WillRepeatedly(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);

    uint64_t ns_sum = (std::chrono::duration_cast<std::chrono::nanoseconds>(ts.seconds) + ts.nanoseconds).count();
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(ns_sum)));
    auto ts_read = stbc->GetCurrentTime();

    // 3. Set Up for function GetCurrentTime for consumer2
    auto reader_mock2 = static_cast<SharedMemTimeBaseReaderMock *>(stbc2->time_base_reader_.get());
    score::time::TimestampWithStatus ts2 = {SynchronizationStatus::kNotSynchronizedUntilStartup,
                                           std::chrono::nanoseconds(12345), std::chrono::seconds(43)};
    VirtualLocalTime lt2(ns_diff);

    EXPECT_CALL(*reader_mock2, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock2));
    EXPECT_CALL(*reader_mock2, GetName()).WillRepeatedly(Return(shmem_name2));
    EXPECT_CALL(*reader_mock2, Open()).Times(2);
    EXPECT_CALL(*reader_mock2, lock()).Times(2);
    EXPECT_CALL(*reader_mock2, Read(An<score::time::TimestampWithStatus &>()))
        .WillRepeatedly(DoAll(SetArgReferee<0>(ts2), Return(true)));
    EXPECT_CALL(*reader_mock2, Read(An<VirtualLocalTime &>()))
        .WillRepeatedly(DoAll(SetArgReferee<0>(lt2), Return(true)));
    EXPECT_CALL(*reader_mock2, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock2, unlock()).Times(2);

    uint64_t ns_sum2 = (std::chrono::duration_cast<std::chrono::nanoseconds>(ts2.seconds) + ts2.nanoseconds).count();
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(ns_sum2)));
    auto ts_read2 = stbc2->GetCurrentTime();
    // 4. Make sure that there no Segmentation fault
    delete (stbc);
    delete (stbc2);
    EXPECT_LE(ts_read.time_since_epoch(), ts_read2.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetCurrentTime_OnReaderFailInvalidVlt_SucceedsWithZeroTimestamp) {
    std::string_view shmem_name("/two");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer2"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<VirtualLocalTime &>())).WillOnce(Return(false));

    auto ts1 = stbc.GetCurrentTime();

    ASSERT_EQ(Timestamp::duration::zero(), ts1.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetCurrentTime_WithStatuskNotSynchronizedUntilStartup_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());
    score::time::TimestampWithStatus ts = {SynchronizationStatus::kNotSynchronizedUntilStartup,
                                          std::chrono::nanoseconds(0), std::chrono::seconds(43)};

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Open()).Times(1);
    EXPECT_CALL(*reader_mock, lock()).Times(1);
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kNotSynchronizedUntilStartup), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(1);

    // return real reader for handling of timebase mappings
    // reader_factory_mock_return_real_reader = true;
    uint64_t ns_sum = (std::chrono::duration_cast<std::chrono::nanoseconds>(ts.seconds) + ts.nanoseconds).count();
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(ns_sum)));
    auto ts1 = stbc.GetCurrentTime();
    EXPECT_EQ(ts1.time_since_epoch(), std::chrono::nanoseconds(ns_sum));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetCurrentTime_WithGetCurrentVirtualLocalTimeFailed_Failed) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Open()).Times(1);
    EXPECT_CALL(*reader_mock, lock()).Times(1);
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kNotSynchronizedUntilStartup), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(1);

    // return real reader for handling of timebase mappings
    // reader_factory_mock_return_real_reader = true;
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::nullopt));
    auto ts1 = stbc.GetCurrentTime();
    EXPECT_EQ(ts1.time_since_epoch(), Timestamp::duration::zero());
}

// TODO: The following 4 tests could be converted to a parametrized test
TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    auto ud_arr = make_array<std::byte>(1, 2, 3);
    score::cpp::span<const std::byte> ud_span = ud_arr;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*reader_mock, Read(An<score::cpp::span<const std::byte> &>()))
        .WillOnce(DoAll(SetArgReferee<0>(ud_span), Return(true)));

    auto status = stbc.GetTimeWithStatus();
    auto ud = status.GetUserData();

    ASSERT_EQ(ud, ud_span);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_WithEmptyUserData_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    score::cpp::span<const std::byte> ud_span;
    score::cpp::span<const std::byte> empty_span;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*reader_mock, Read(empty_span)).WillOnce(DoAll(SetArgReferee<0>(ud_span), Return(true)));

    auto status = stbc.GetTimeWithStatus();
    auto ud = status.GetUserData();

    ASSERT_TRUE(ud.empty());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_WithOneByteUserData_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    auto ud_arr = make_array<std::byte>(1);
    score::cpp::span<const std::byte> ud_span = ud_arr;
    score::cpp::span<const std::byte> empty_span;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*reader_mock, Read(empty_span)).WillOnce(DoAll(SetArgReferee<0>(ud_span), Return(true)));

    auto status = stbc.GetTimeWithStatus();
    auto ud = status.GetUserData();

    ASSERT_EQ(ud, ud_span);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_WithTwoByteUserData_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    auto ud_arr = make_array<std::byte>(1, 2);
    score::cpp::span<const std::byte> ud_span = ud_arr;
    score::cpp::span<const std::byte> empty_span;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*reader_mock, Read(empty_span)).WillOnce(DoAll(SetArgReferee<0>(ud_span), Return(true)));

    auto status = stbc.GetTimeWithStatus();
    auto ud = status.GetUserData();

    ASSERT_EQ(ud, ud_span);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnNotSynchronized_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // None of the status bits set
    SynchronizationStatus status_to_inject = SynchronizationStatus::kNotSynchronizedUntilStartup;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(1000)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kNotSynchronizedUntilStartup, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnSynchronizedWithCurrentTime_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // None of the status bits set
    SynchronizationStatus status_to_inject = SynchronizationStatus::kSynchronized;
    TimestampWithStatus ts{};
    VirtualLocalTime lt{};

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<TimestampWithStatus &>())).WillOnce(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<VirtualLocalTime &>())).WillOnce(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(VirtualLocalTime{}));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kSynchronized, status.GetSynchronizationStatus());
    ASSERT_EQ(status.GetCreationTime().time_since_epoch().count(), 0u);
}

TEST_F(SynchronizedTimeBaseConsumerFixture,
       GetTimeWithStatus_OnSynchronizedWithNoCurrentTime_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    SynchronizationStatus status_to_inject = SynchronizationStatus::kSynchronized;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<TimestampWithStatus &>())).WillOnce(Return(false));

    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kSynchronized, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture,
       GetTimeWithStatus_WithGetCurrentVirtualLocalTimeFailed_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // None of the status bits set
    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>())).WillOnce(Return(false));
    EXPECT_CALL(*shared_utils_mock, GetCurrentVirtualLocalTime()).WillOnce(Return(std::chrono::nanoseconds(1000)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kNotSynchronizedUntilStartup, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnTimeout_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // Mock Timeout
    SynchronizationStatus status_to_inject = SynchronizationStatus::kTimeOut;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kTimeOut, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnNotSynchronizedToGateway_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // Mock SynctoGateway
    SynchronizationStatus status_to_inject = SynchronizationStatus::kSynchToGateway;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kSynchToGateway, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnSynchronized_SucceedsWithCorrectStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // Mock GlobalSync
    SynchronizationStatus status_to_inject = SynchronizationStatus::kSynchronized;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kSynchronized, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnReadStatusFailure_FailWithInvalidStatus) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    // Mock GlobalSync
    SynchronizationStatus status_to_inject = SynchronizationStatus::kNotSynchronizedUntilStartup;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus &>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(false)));
    auto status = stbc.GetTimeWithStatus();

    ASSERT_EQ(SynchronizationStatus::kNotSynchronizedUntilStartup, status.GetSynchronizationStatus());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetTimeWithStatus_OnEmptyUserData_Succeeds) {
    std::string_view shmem_name("/one");
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    score::cpp::span<const std::byte> empty_span;

    EXPECT_CALL(*reader_mock, GetAccessor()).WillRepeatedly(ReturnRef(*reader_mock));
    EXPECT_CALL(*reader_mock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*reader_mock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*reader_mock, Read(empty_span)).WillOnce(Return(false));

    auto status = stbc.GetTimeWithStatus();
    auto ud = status.GetUserData();

    ASSERT_EQ(ud, empty_span);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, GetRateDeviation_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act
    auto result = stbc.GetRateDeviation();

    // Assert
    ASSERT_EQ(result, 0.0);
}

TEST_F(SynchronizedTimeBaseConsumerFixture, RegisterStatusChangeNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.RegisterStatusChangeNotifier(nullptr));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, UnregisterStatusChangeNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.UnregisterStatusChangeNotifier());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, RegisterSynchronizationStateChangeNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.RegisterSynchronizationStateChangeNotifier(nullptr));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, UnregisterSynchronizationStateChangeNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.UnregisterSynchronizationStateChangeNotifier());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, RegisterTimeLeapNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.RegisterTimeLeapNotifier(nullptr));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, UnregisterTimeLeapNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.UnregisterTimeLeapNotifier());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, RegisterTimeValidationNotification_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    ConsumerTimeBaseValidationNotificationMock notification;

    // Act and assert
    EXPECT_NO_THROW(stbc.RegisterTimeValidationNotification(notification));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, UnregisterTimeValidationNotification_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.UnregisterTimeValidationNotification());
}

TEST_F(SynchronizedTimeBaseConsumerFixture, RegisterTimePrecisionMeasurementNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.RegisterTimePrecisionMeasurementNotifier(nullptr));
}

TEST_F(SynchronizedTimeBaseConsumerFixture, UnregisterTimePrecisionMeasurementNotifier_Succeeds) {
    // Arrange
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));

    // Act and assert
    EXPECT_NO_THROW(stbc.UnregisterTimePrecisionMeasurementNotifier());
}

TEST_F(SynchronizedTimeBaseCommonFixture, GetUserData_OnAlignmentFailure_Fails) {
    SynchronizedTimeBaseConsumer stbc(InstanceSpecifier("consumer1"));
    // defaults are checked
    ASSERT_NE(stbc.time_base_reader_.get(), nullptr);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock *>(stbc.time_base_reader_.get());

    EXPECT_CALL(*reader_mock, GetPosition()).WillRepeatedly(testing::Return(4097));
    auto res = SynchronizedTimeBaseCommon::GetUserData(*reader_mock);

    ASSERT_FALSE(res.has_value());
}

}  // namespace synchronizedtimebaseconsumer_ut
}  // namespace testing
