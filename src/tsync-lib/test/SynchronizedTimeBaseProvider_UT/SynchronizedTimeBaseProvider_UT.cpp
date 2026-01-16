/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gmock/gmock.h>

#include <csignal>
#include <string_view>
#include <type_traits>

#include "SynchronizedTimeBaseCommon.h"
#include "score/time/time_error_domain.h"
#include "matcher_operators.h"
#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseWriterMock.h"
#include "TimeBaseReaderFactoryMock.h"
#include "TimeBaseWriterFactoryMock.h"
#include "TsyncSharedUtilsMock.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncNamedSemaphore.h"
#include "score/time/utility/TsyncSharedUtils.h"

#define private public
// class under test
#include "score/time/synchronized_time_base_provider.h"
#undef private

using InstanceSpecifier = std::string_view;

using namespace score::time;
using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;

namespace score {
namespace time {
// defined in SynchronizedTimeBaseCommon.cpp
extern TsyncIdMappingsHandler mappings_handler;
}  // namespace time
}  // namespace score

using score::time::mappings_handler;

namespace score {
namespace cpp {
bool operator==(const score::cpp::span<const std::byte>& v1, const score::cpp::span<const std::byte>& v2) {
    return (std::equal(std::begin(v1), std::end(v1), std::begin(v2), std::end(v2)));
}

bool operator!=(const score::cpp::span<const std::byte>& v1, const score::cpp::span<const std::byte>& v2) {
    return (!(v1 == v2));
}
}  // namespace cpp
}  // namespace score

class SynchronizedTimeBaseProviderFixture : public ::testing::Test {
public:
    void SetUp() override {
        GTEST_SKIP() << "Skipping all tests for this fixture";
        shared_utils_mock = std::make_unique<::testing::NiceMock<TsyncSharedUtilsMock>>();
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();
        mappings_handler.Clear();
        ASSERT_TRUE(mappings_handler.AddDomainMapping(1U, "one"));
        ASSERT_TRUE(mappings_handler.AddDomainMapping(2U, "two"));
        ASSERT_TRUE(mappings_handler.AddProviderToDomain(1U, "provider1"));
        ASSERT_TRUE(mappings_handler.AddProviderToDomain("two", "provider2"));
        reader_factory_mock_return_real_reader = true;
        writer_factory_mock_return_real_writer = true;
        mappings_handler.CommitMappingsToSharedMemory();
        mappings_handler.DoSharedMemoryRead();
        ASSERT_FALSE(mappings_handler.IsEmpty());
        reader_factory_mock_return_real_reader = false;
        writer_factory_mock_return_real_writer = false;

        std::signal(SIGABRT, &AbortHandler);
        ::testing::Mock::AllowLeak(shared_utils_mock.get());
        ::testing::Mock::AllowLeak(reader_factory_mock.get());
        ::testing::Mock::AllowLeak(writer_factory_mock.get());
    }

    void TearDown() override {
        CleanUp();
        std::signal(SIGABRT, SIG_DFL);
    }

    static void CleanUp() {
        shared_utils_mock.reset();
        reader_factory_mock.reset();
        writer_factory_mock.reset();
    }

    static void AbortHandler(int /*signal*/) noexcept {
        CleanUp();
        std::exit(EXIT_CODE);
    }

    static const int32_t EXIT_CODE;
};

const int32_t SynchronizedTimeBaseProviderFixture::EXIT_CODE = 1;

// This fixture is for testing the setting of user data of different lengths. It will be
// parametrized with values 0..4
class SynchronizedTimeBaseProviderSetUserDataFixture : public ::testing::TestWithParam<unsigned char> {
public:
    void SetUp() override {
        GTEST_SKIP() << "Skipping all tests for this fixture";
        shared_utils_mock = std::make_unique<::testing::NiceMock<TsyncSharedUtilsMock>>();
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();
        mappings_handler.Clear();
        ASSERT_TRUE(mappings_handler.AddDomainMapping(1U, "one"));
        ASSERT_TRUE(mappings_handler.AddDomainMapping(2U, "two"));
        ASSERT_TRUE(mappings_handler.AddProviderToDomain(1U, "provider1"));
        ASSERT_TRUE(mappings_handler.AddProviderToDomain("two", "provider2"));
        reader_factory_mock_return_real_reader = true;
        writer_factory_mock_return_real_writer = true;
        mappings_handler.CommitMappingsToSharedMemory();
        mappings_handler.DoSharedMemoryRead();
        reader_factory_mock_return_real_reader = false;
        writer_factory_mock_return_real_writer = false;
    }

    void TearDown() override {
        shared_utils_mock.reset();
        reader_factory_mock.reset();
        writer_factory_mock.reset();
    }
};

namespace testing {
namespace synchronizedtimebaseprovider_ut {

TEST_F(SynchronizedTimeBaseProviderFixture, Ctor_Succeeds) {
    // std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    // EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
    //     .WillRepeatedly(Return(std::string("time_domain_1")));
    // dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
    //                                                    TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    ASSERT_NE(stbp.time_base_writer_.get(), nullptr);
}

TEST_F(SynchronizedTimeBaseProviderFixture, Ctor_OnInvalidProviderName_Aborts) {
    ASSERT_EXIT({ SynchronizedTimeBaseProvider stbp(::InstanceSpecifier("invalid")); },
                ::testing::ExitedWithCode(EXIT_CODE), "couldn't find time domain for provider");
}

TEST_F(SynchronizedTimeBaseProviderFixture, GetTimeBaseDomainId_Succeeds) {
    auto domain_id = score::time::SynchronizedTimeBaseCommon::GetTimeBaseDomainId("/one");
    EXPECT_EQ(1U, domain_id);
}

TEST_F(SynchronizedTimeBaseProviderFixture, MoveCtor_Succeeds) {
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp1(InstanceSpecifier("provider1"));
    auto writer = stbp1.time_base_writer_.get();
    auto reader = stbp1.time_base_reader_.get();
    ASSERT_NE(writer, nullptr);
    ASSERT_NE(reader, nullptr);
    SynchronizedTimeBaseProvider stbp2(std::move(stbp1));
    ASSERT_EQ(stbp1.time_base_writer_, nullptr);
    ASSERT_EQ(stbp1.time_base_reader_, nullptr);
    ASSERT_EQ(writer, stbp2.time_base_writer_.get());
    ASSERT_EQ(reader, stbp2.time_base_reader_.get());
    dummy_sem1.reset();
}

TEST_F(SynchronizedTimeBaseProviderFixture, MoveAssignment_Succeeds) {
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp1(InstanceSpecifier("provider1"));
    auto writer = stbp1.time_base_writer_.get();
    auto reader = stbp1.time_base_reader_.get();
    ASSERT_NE(writer, nullptr);
    ASSERT_NE(reader, nullptr);
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem2;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_2")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp2(InstanceSpecifier("provider2"));
    ASSERT_NE(writer, stbp2.time_base_writer_.get());
    ASSERT_NE(reader, stbp2.time_base_reader_.get());
    stbp2 = std::move(stbp1);
    ASSERT_EQ(stbp1.time_base_writer_, nullptr);
    ASSERT_EQ(stbp1.time_base_reader_, nullptr);
    ASSERT_EQ(writer, stbp2.time_base_writer_.get());
    ASSERT_EQ(reader, stbp2.time_base_reader_.get());
    dummy_sem1.reset();
    dummy_sem2.reset();
}

TEST_F(SynchronizedTimeBaseProviderFixture, GetCurrentTime_Succeeds) {
    std::string_view shmem_name("/one");
    // Set up for constructor
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));

    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());
    TimestampWithStatus ts = {SynchronizationStatus::kNotSynchronizedUntilStartup, std::chrono::nanoseconds(0),
                              std::chrono::seconds(43)};
    std::chrono::nanoseconds ns_inject =
        ts.nanoseconds + std::chrono::duration_cast<std::chrono::nanoseconds>(ts.seconds);
    const uint32_t ns_diff{100000U};
    VirtualLocalTime lt(ns_diff);

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*readerMock, Read(An<VirtualLocalTime&>())).WillOnce(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*readerMock, unlock()).Times(1);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(ns_inject));

    // return real reader for handling of timebase mappings
    auto ts1 = stbp.GetCurrentTime();
    reader_factory_mock_return_real_reader = false;
    ;

    auto ns_sum = ns_inject + std::chrono::nanoseconds(ns_diff);

    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*readerMock, Read(An<VirtualLocalTime&>())).WillOnce(DoAll(SetArgReferee<0>(lt), Return(true)));
    EXPECT_CALL(*readerMock, unlock()).Times(1);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(ns_sum));

    auto ts2 = stbp.GetCurrentTime();

    std::chrono::nanoseconds nsHelper(ns_diff);

    auto d_cmp =
        std::chrono::duration_cast<Timestamp::duration>(ts1.time_since_epoch() + std::chrono::nanoseconds(ns_diff));

    ASSERT_EQ(d_cmp.count(), ts2.time_since_epoch().count());
}

TEST_F(SynchronizedTimeBaseProviderFixture, GetCurrentTime_OnReaderFail_ReturnsZeroTime) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());
    TimestampWithStatus ts = {};

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(DoAll(SetArgReferee<0>(ts), Return(true)));
    EXPECT_CALL(*readerMock, Read(An<VirtualLocalTime&>())).WillOnce(Return(false));
    EXPECT_CALL(*readerMock, unlock()).Times(1);

    // return real reader for handling of timebase mappings
    reader_factory_mock_return_real_reader = true;
    auto ts1 = stbp.GetCurrentTime();
    reader_factory_mock_return_real_reader = false;
    ;

    ASSERT_EQ(Timestamp::duration::zero(), ts1.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseProviderFixture, GetCurrentTime_OnTimeStampReadFailure_ReturnsZeroTime) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbc(InstanceSpecifier("provider1"));
    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbc.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(Return(false));
    auto ts1 = stbc.GetCurrentTime();
    ASSERT_EQ(Timestamp::duration::zero(), ts1.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseProviderFixture, UpdateTime_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());
    uint32_t ns_diff = 100000;
    VirtualLocalTime vlt_to_inject(ns_diff);

    std::chrono::nanoseconds ts_ns(56789);
    Timestamp ts{ts_ns};
    auto tst = SynchronizedTimeBaseCommon::GetTimestampFromNs(ts_ns);
    tst.status = SynchronizationStatus::kSynchronized;
    auto span_data = make_array<std::byte>(3, 1, 2, 3);
    score::cpp::span<const std::byte> byte_span(span_data);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, Open()).Times(1);
    EXPECT_CALL(*writerMock, lock()).Times(1);
    EXPECT_CALL(*writerMock, Write(Matcher<const TimestampWithStatus&>(tst))).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<const VirtualLocalTime&>(vlt_to_inject))).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<score::cpp::span<const std::byte>>(_))).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, unlock()).Times(1);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject));

    auto res = stbp.UpdateTime(ts, byte_span);
    ASSERT_TRUE(res);

    // a bit of a sanity check here to verify symmetry of UpdateTime and GetCurrentTime
    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(DoAll(SetArgReferee<0>(tst), Return(true)));
    EXPECT_CALL(*readerMock, Read(An<VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject), Return(true)));
    EXPECT_CALL(*readerMock, unlock()).Times(1);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject));

    auto ts_read = stbp.GetCurrentTime();

    ASSERT_EQ(ts.time_since_epoch(), ts_read.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseProviderFixture, UpdateTime_WhenCalledTwice_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    uint32_t ns_diff = 100000;
    VirtualLocalTime vlt_to_inject(ns_diff);

    std::chrono::nanoseconds ts_ns(56789);
    Timestamp ts{ts_ns};
    auto tst = SynchronizedTimeBaseCommon::GetTimestampFromNs(ts_ns);
    tst.status = SynchronizationStatus::kSynchronized;
    auto span_data = make_array<std::byte>(3, 1, 2, 3);
    score::cpp::span<const std::byte> byte_span(span_data);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillRepeatedly(Return(vlt_to_inject));
    EXPECT_CALL(*writerMock, Open()).Times(2);
    EXPECT_CALL(*writerMock, lock()).Times(2);
    EXPECT_CALL(*writerMock, Write(Matcher<const TimestampWithStatus&>(tst))).WillRepeatedly(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<const VirtualLocalTime&>(vlt_to_inject))).WillRepeatedly(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<UserDataView>(_))).WillRepeatedly(Return(true));
    EXPECT_CALL(*writerMock, unlock()).Times(2);

    auto res = stbp.UpdateTime(ts, byte_span);
    ASSERT_TRUE(res);

    res = stbp.UpdateTime(ts, byte_span);
    ASSERT_TRUE(res);

    // a bit of a sanity check here to verify symmetry of UpdateTime and GetCurrentTime
    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, Read(An<TimestampWithStatus&>())).WillOnce(DoAll(SetArgReferee<0>(tst), Return(true)));
    EXPECT_CALL(*readerMock, Read(An<VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject), Return(true)));
    EXPECT_CALL(*readerMock, unlock()).Times(1);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject));

    auto ts_read = stbp.GetCurrentTime();

    ASSERT_EQ(ts.time_since_epoch(), ts_read.time_since_epoch());
}

TEST_F(SynchronizedTimeBaseProviderFixture, UpdateTime_OnGetVltError_ReturnsError) {
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    // EXPECT_CALL(*writerMock, GetName()).WillOnce(Return("/one"));
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(std::nullopt));

    Timestamp ts{std::chrono::nanoseconds(56789)};
    auto res = stbp.UpdateTime(ts);
    ASSERT_FALSE(res);
    ASSERT_EQ(res.error(), TimeErrorCode::kDaemonConnectionLost);
}

TEST_F(SynchronizedTimeBaseProviderFixture, UpdateTime_OnVltReadError_ReturnsError) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    TimestampWithStatus tst = {};
    tst.status = SynchronizationStatus::kSynchronized;
    VirtualLocalTime vlt_to_inject(0);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*writerMock, Open()).Times(1);
    EXPECT_CALL(*writerMock, lock()).Times(1);
    EXPECT_CALL(*writerMock, Write(Matcher<const TimestampWithStatus&>(tst))).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<const VirtualLocalTime&>(vlt_to_inject))).WillOnce(Return(false));
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject));
    EXPECT_CALL(*writerMock, unlock()).Times(1);

    Timestamp ts{std::chrono::nanoseconds(0)};

    auto res = stbp.UpdateTime(ts);
    ASSERT_FALSE(res);
    ASSERT_EQ(res.error(), TimeErrorCode::kDaemonConnectionLost);
}

TEST_F(SynchronizedTimeBaseProviderFixture, GetUserData_OnError_ReturnsEmptyUserData) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Read(An<UserDataView&>())).WillOnce(Return(false));

    auto res = stbp.GetUserData();
    ASSERT_EQ(res.size(), 0u);
}

TEST_F(SynchronizedTimeBaseProviderFixture, SetTime_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    uint32_t ns_diff = 100000;
    VirtualLocalTime vlt_to_inject(ns_diff);

    std::chrono::nanoseconds ts_ns(56789);
    Timestamp ts{ts_ns};
    auto tst = SynchronizedTimeBaseCommon::GetTimestampFromNs(ts_ns);
    tst.status = SynchronizationStatus::kSynchronized;

    auto span_data = make_array<std::byte>(3, 1, 2, 3);
    score::cpp::span<const std::byte> byte_span(span_data);

    // expected calls from successful UpdateTime call
    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*writerMock, Open()).Times(1);
    EXPECT_CALL(*writerMock, lock()).Times(1);
    EXPECT_CALL(*writerMock, Write(Matcher<const TimestampWithStatus&>(tst))).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, Write(Matcher<const VirtualLocalTime&>(vlt_to_inject))).WillOnce(Return(true));
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject));
    EXPECT_CALL(*writerMock, Write(Matcher<UserDataView>(_))).WillOnce(Return(true));

    EXPECT_CALL(*writerMock, unlock()).Times(1);

    auto res = stbp.SetTime(ts, byte_span);
    ASSERT_TRUE(res);
}

TEST_F(SynchronizedTimeBaseProviderFixture, SetUserData_OnWriterSetPositionFailure_ReturnsError) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    EXPECT_CALL(*writerMock, GetAccessor()).WillOnce(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*writerMock, GetPosition()).WillRepeatedly(::Return(43u));  // unaligned condition, forcing SetPosition
    EXPECT_CALL(*writerMock, SetPosition(_)).WillRepeatedly(::Return(false));

    score::cpp::span<const std::byte> user_data;

    auto res = stbp.SetUserData(user_data);
    ASSERT_FALSE(res);
    ASSERT_EQ(res.error(), TimeErrorCode::kDaemonConnectionLost);
}

TEST_F(SynchronizedTimeBaseProviderFixture, SetTime_OnUpdateTimeHaveNoValue_ReturnsError) {
    std::string_view shmem_name("one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    std::chrono::nanoseconds ts_ns(56789);
    Timestamp ts{ts_ns};
    auto span_data = make_array<std::byte>(3, 1, 2, 3);
    score::cpp::span<const std::byte> byte_span(span_data);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return("one"));
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(std::nullopt));

    auto res = stbp.SetTime(ts, byte_span);
    ASSERT_FALSE(res);
}

}  // namespace lib_synchronizedtimebaseprovider_ut

class ProviderTimeBaseValidationNotificationImpl final : public ProviderTimeBaseValidationNotification {
public:
    void SetPdelayResponderData(const PdelayResponderMeasurementType&) override {
    }

    void SetMasterTimingData(const TimeMasterMeasurementType&) override {
    }
};

namespace lib_synchronizedtimebaseprovider_ut {

TEST_F(SynchronizedTimeBaseProviderFixture, RegisterTimeValidationNotification_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    ProviderTimeBaseValidationNotificationImpl notification;
    stbp.RegisterTimeValidationNotification(notification);
    stbp.UnregisterTimeValidationNotification();
    SUCCEED();
}

TEST_P(SynchronizedTimeBaseProviderSetUserDataFixture, SetGetUserData_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    auto ud_len = GetParam();
    auto span_data = new std::byte[ud_len];

    for (int i = 0; i < ud_len; ++i) {
        span_data[i] = std::byte(i + 1);
    }

    score::cpp::span<const std::byte> byte_span(span_data, ud_len);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*writerMock, Open()).Times(1);
    EXPECT_CALL(*writerMock, lock()).Times(1);
    EXPECT_CALL(*writerMock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*writerMock, Write(An<UserDataView>())).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, unlock()).Times(1);

    auto res = stbp.SetUserData(byte_span);
    ASSERT_TRUE(res);

    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*readerMock, Read(An<score::cpp::span<const std::byte>&>())).WillOnce(Return(true));
    EXPECT_CALL(*readerMock, unlock()).Times(1);

    stbp.GetUserData();

    delete[] span_data;
}

}  // namespace lib_synchronizedtimebaseprovider_ut

INSTANTIATE_TEST_SUITE_P(SynchronizedTimeBaseProviderUserDataTests, SynchronizedTimeBaseProviderSetUserDataFixture,
                         testing::Values(0, 1, 2, 3, 4));

namespace lib_synchronizedtimebaseprovider_ut {

TEST_F(SynchronizedTimeBaseProviderFixture, SetGetUserData_Succeeds) {
    std::string_view shmem_name("/one");
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));
    auto writerMock = static_cast<SharedMemTimeBaseWriterMock*>(stbp.time_base_writer_.get());

    auto span_data = make_array<std::byte>(3, 1, 2, 3);
    score::cpp::span<const std::byte> byte_span(span_data);

    EXPECT_CALL(*writerMock, GetAccessor()).WillRepeatedly(ReturnRef(*writerMock));
    EXPECT_CALL(*writerMock, GetName()).WillRepeatedly(Return(shmem_name));
    EXPECT_CALL(*writerMock, Open()).Times(1);
    EXPECT_CALL(*writerMock, lock()).Times(1);
    EXPECT_CALL(*writerMock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*writerMock, Write(An<UserDataView>())).WillOnce(Return(true));
    EXPECT_CALL(*writerMock, unlock()).Times(1);

    auto res = stbp.SetUserData(byte_span);
    ASSERT_TRUE(res);

    auto readerMock = static_cast<SharedMemTimeBaseReaderMock*>(stbp.time_base_reader_.get());

    EXPECT_CALL(*readerMock, GetAccessor()).WillRepeatedly(ReturnRef(*readerMock));
    EXPECT_CALL(*readerMock, Open()).Times(1);
    EXPECT_CALL(*readerMock, lock()).Times(1);
    EXPECT_CALL(*readerMock, SetPosition(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*readerMock, Read(An<score::cpp::span<const std::byte>&>()))
        .WillOnce(DoAll(SetArgReferee<0>(byte_span), Return(true)));
    EXPECT_CALL(*readerMock, unlock()).Times(1);

    auto read_ud = stbp.GetUserData();

    ASSERT_EQ(byte_span, read_ud);
}

// // TODO: implement this test alongside with the GetRateDeviation.
TEST_F(SynchronizedTimeBaseProviderFixture, GetRateDeviation_Succeeds) {
    // Arrange
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));

    // Act
    auto result = stbp.GetRateDeviation();

    // Assert
    ASSERT_EQ(result, 0.0);
}

// // TODO: implement this test alongside with the SetRateCorrection.
TEST_F(SynchronizedTimeBaseProviderFixture, SetRateCorrection_Succeeds) {
    // Arrange
    std::unique_ptr<TsyncNamedSemaphore> dummy_sem1;
    EXPECT_CALL(*shared_utils_mock.get(), GetTransmissionSemaphoreName(_))
        .WillRepeatedly(Return(std::string("time_domain_1")));
    dummy_sem1 = std::make_unique<TsyncNamedSemaphore>(TsyncSharedUtils::GetTransmissionSemaphoreName(1),
                                                       TsyncNamedSemaphore::OpenMode::Unsignaled, true);
    SynchronizedTimeBaseProvider stbp(InstanceSpecifier("provider1"));

    // Act
    auto result = stbp.SetRateCorrection(1);

    // Assert
    ASSERT_TRUE(result);
}

}  // namespace synchronizedtimebaseprovider_ut
}  // namespace testing
