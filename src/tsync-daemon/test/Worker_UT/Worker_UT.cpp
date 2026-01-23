/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "score/time/utility/TsyncIdMappingsHandler.h"

#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseWriterMock.h"
#include "SysCallsNamedSemMock.h"
#include "SysCallsMiscMock.h"
#include "TimeBaseReaderFactoryMock.h"
#include "TimeBaseWriterFactoryMock.h"
#include "TsyncSharedUtilsMock.h"

#include "HouseKeeping.h"
#include "TimeBaseConfiguration.h"

#define private public
#include "TsyncWorker.h"
#undef public

using namespace score::time;
using namespace std::chrono_literals;

using score::time::daemon::HouseKeeping;
using score::time::daemon::TimeBaseConfigData;
using score::time::daemon::TimeBaseConfiguration;

using ::testing::_;
using ::testing::An;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace testing {
namespace daemon_worker_ut {

class DaemonWorkerFixture : public ::testing::Test {
public:
    static const int32_t EXIT_CODE;

protected:
    void SetUp() override {
        writer_factory_mock_return_real_writer = false;
        reader_factory_mock_return_real_reader = false;
        shared_utils_mock = std::make_unique<::testing::NiceMock<TsyncSharedUtilsMock>>();
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();
        misc_mock = std::make_unique<::testing::NiceMock<SysCallsMiscMock>>();
        named_semaphore_mock = std::make_unique<::testing::NiceMock<SysCallsNamedSemMock>>();

        TimeBaseConfigData cfg{"TEST", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}};
        cfg.timebase_config.provider_config.time_master_config.is_valid = true;
        TimeBaseConfiguration::GetInstance().AddConfigData(cfg);

        // install abort handler for our death tests
        std::signal(SIGABRT, &AbortHandler);
        // As we use singleton mock objects, clear expectations after each test
        ::testing::Mock::AllowLeak(misc_mock.get());
        ::testing::Mock::AllowLeak(shared_utils_mock.get());
        ::testing::Mock::AllowLeak(reader_factory_mock.get());
        ::testing::Mock::AllowLeak(writer_factory_mock.get());
    }

    void TearDown() override {
        CleanUp();
        std::signal(SIGABRT, SIG_DFL);
    }

    static void CleanUp() {
        TimeBaseConfiguration::GetInstance().Clear();

        // these mocks have to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        reader_factory_mock.reset();
        writer_factory_mock.reset();
        misc_mock.reset();
        shared_utils_mock.reset();
        worker_.ShutDown();
    }

    static void AbortHandler(int /*signal*/) noexcept {
        CleanUp();
        std::exit(EXIT_CODE);
    }

    static score::time::daemon::TsyncWorker worker_;
};

 score::time::daemon::TsyncWorker DaemonWorkerFixture::worker_{};

const int32_t DaemonWorkerFixture::EXIT_CODE = 1;

TEST_F(DaemonWorkerFixture, Init_NotInitialized_Success) {
    // Arrange
    InSequence seq;

    // Act
    worker_.Init();

    // Assert
    EXPECT_TRUE(worker_.is_initialized_);
}

TEST_F(DaemonWorkerFixture, Init_AlreadyInitialized_Success) {
    // Arrange
    InSequence seq;

    worker_.Init();

    // Act and Assert
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);
}

TEST_F(DaemonWorkerFixture, Init_OnCommitMappingsToSharedMemoryFailure_Aborts) {
    ASSERT_EXIT(
        {
            // Arrange
            auto writer_mock = writer_factory_mock->Create(kIdMappingsShmemFileName, kIdMappingsShmemSize, true);
            auto raw_writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(writer_mock.get());
            ::testing::Mock::AllowLeak(raw_writer_mock);
            EXPECT_CALL(*raw_writer_mock, Write(::testing::An<uint32_t>())).WillOnce(::testing::Return(false));
            EXPECT_CALL(*writer_factory_mock, Create(::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(testing::ByMove(std::move(writer_mock))));

            // Act
            worker_.Init();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing time domain mapping");
}

TEST_F(DaemonWorkerFixture, Init_OnAddDomainMappingFailure_Aborts) {
    // Arrange
    // Same domain id as in SetUp
    TimeBaseConfigData cfg{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 1}};
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg);

    ASSERT_EXIT(
        {
            auto writer_mock = writer_factory_mock->Create(kIdMappingsShmemFileName, kIdMappingsShmemSize, true);
            auto raw_writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(writer_mock.get());
            ::testing::Mock::AllowLeak(raw_writer_mock);
            EXPECT_CALL(*raw_writer_mock, Write(::testing::An<uint32_t>())).WillOnce(::testing::Return(false));
            EXPECT_CALL(*writer_factory_mock, Create(::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(testing::ByMove(std::move(writer_mock))));

            // Act
            worker_.Init();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error adding domain mapping");
}

TEST_F(DaemonWorkerFixture, Run_RunUntilExit_Success) {
    // Arrange
    InSequence seq;

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    // Assert
    // This should make the Run method finish immediately
    HouseKeeping::exit_flag_ = 1;
    worker_.Run();
    SharedMemTimeBaseWriterMock* mock_ptr =
        static_cast<SharedMemTimeBaseWriterMock*>(worker_.time_base_writers_[0].get());
    EXPECT_CALL(*mock_ptr, Close()).Times(1);
    worker_.ShutDown();
}

TEST_F(DaemonWorkerFixture, Run_WithTimeoutCheck_Succeeds) {
    // Arrange
    TimeBaseConfigData cfg_valid{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};
    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid.timebase_config.consumer_config.time_slave_config.is_valid = true;
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid);
    InSequence seq;

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);
    HouseKeeping::exit_flag_ = 0;

    std::thread exit_toggler_thread([]() {
        std::this_thread::sleep_for(5000ms);
        HouseKeeping::exit_flag_ = 1;
    });

    exit_toggler_thread.detach();
    worker_.Run();
    SharedMemTimeBaseWriterMock* mock_ptr =
        static_cast<SharedMemTimeBaseWriterMock*>(worker_.time_base_writers_[0].get());
    EXPECT_CALL(*mock_ptr, Close()).Times(1);
    worker_.ShutDown();
}

TEST_F(DaemonWorkerFixture, Run_WithTimeStampSkipError_Aborts) {
    // Arrange
    TimeBaseConfigData cfg_valid{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};
    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid.timebase_config.consumer_config.time_slave_config.is_valid = true;
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid);

    ASSERT_EXIT(
        {
            InSequence seq;

            // Act
            worker_.Init();
            EXPECT_TRUE(worker_.is_initialized_);

            HouseKeeping::exit_flag_ = 0;

            std::thread exit_toggler_thread([]() {
                std::this_thread::sleep_for(5000ms);
                HouseKeeping::exit_flag_ = 1;
            });

            exit_toggler_thread.detach();
            auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());
            EXPECT_CALL(*raw_reader_mock, SetPosition(_)).WillOnce(::testing::Return(false));
            worker_.Run();
        },
        ::testing::ExitedWithCode(EXIT_FAILURE), "Skip failed");
}

TEST_F(DaemonWorkerFixture, GetTimestampCurrentVlt_Fails) {
    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);
    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(std::nullopt));
    auto res = worker_.GetTimestampCurrentVlt();
    worker_.ShutDown();
    EXPECT_EQ(res.count(), 0);
}

TEST_F(DaemonWorkerFixture, GetTimestampLastUpdateTime_Succeeds) {
    auto p = TimeBaseReaderFactory::Create("TEST");
    SharedMemTimeBaseReaderMock* mock = static_cast<SharedMemTimeBaseReaderMock*>(p.get());
    EXPECT_CALL(*mock, SetPosition(_)).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock, Read(::testing::An<score::time::VirtualLocalTime&>())).WillOnce(::testing::Return(true));

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);
    worker_.GetTimestampLastUpdateTime(p);
}

TEST_F(DaemonWorkerFixture, GetTimestampLastUpdateTime_Read_Fails) {
    auto p = TimeBaseReaderFactory::Create("TEST");
    SharedMemTimeBaseReaderMock* mock = static_cast<SharedMemTimeBaseReaderMock*>(p.get());
    EXPECT_CALL(*mock, SetPosition(_)).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mock, Read(::testing::An<score::time::VirtualLocalTime&>())).WillOnce(::testing::Return(false));

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);
    worker_.GetTimestampLastUpdateTime(p);
}

TEST_F(DaemonWorkerFixture, Run_CallRunBeforeInit_Aborts) {
    // Assert
    ASSERT_EXIT(
        {
            // Act
            worker_.Run();
        },
        ::testing::ExitedWithCode(EXIT_CODE),
        "tsyncd: TsyncWorker::Run - TsyncWorker::Init must be called before TsyncWorker::Run");
}

TEST_F(DaemonWorkerFixture, ShutDown_Return_Success) {
    // This test verifies that the process returns from ShutDown().
    // Act
    worker_.ShutDown();

    // Assert
    // ShutDown() doesn't have return value. Make success with SUCCEED().
    SUCCEED();
}

TEST_F(DaemonWorkerFixture, InitIdMappings_OnAddConsumerFailure_Exit) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";

    TimeBaseConfigData cfg_valid2{"TEST3", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 3}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid2.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid2.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid2.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid2);

    ASSERT_EXIT({ worker_.InitIdMappings(); }, ::testing::ExitedWithCode(EXIT_CODE),
                "Error adding consumer to time domain mapping");
}

TEST_F(DaemonWorkerFixture, InitIdMappings_OnAddProviderFailure_Exit) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.provider_config.time_master_config.is_valid = true;
    cfg_valid1.timebase_config.provider_config.name = "provider";

    TimeBaseConfigData cfg_valid2{"TEST3", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 3}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid2.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid2.timebase_config.provider_config.time_master_config.is_valid = true;
    cfg_valid2.timebase_config.provider_config.name = "provider";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid2);

    ASSERT_EXIT({ worker_.InitIdMappings(); }, ::testing::ExitedWithCode(EXIT_CODE),
                "Error adding provider to time domain mapping");
}

TEST_F(DaemonWorkerFixture, CheckTimeouts_GetTimestampStatusReturnsInvalidStatus_Succeeds) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(-1);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());

    EXPECT_CALL(*reader_mock, lock()).Times(1);
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus&>())).WillOnce(Return(false));
    EXPECT_CALL(*reader_mock, unlock()).Times(1);
    worker_.CheckTimeouts();

    SUCCEED();
}

TEST_F(DaemonWorkerFixture, CheckTimeouts_NoTimeDiff_Succeeds) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(0);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());

    score::time::VirtualLocalTime vlt_to_inject(0);

    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus&>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<score::time::VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);
    worker_.CheckTimeouts();

    SUCCEED();
}

TEST_F(DaemonWorkerFixture, CheckTimeouts_UpdateimeIsGreater_Succeeds) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(0);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());

    score::time::VirtualLocalTime vlt_to_inject(123456);

    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime())
        .WillOnce(Return(std::chrono::nanoseconds(12345)));
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<SynchronizationStatus&>()))
        .WillOnce(DoAll(SetArgReferee<0>(SynchronizationStatus::kSynchronized), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<score::time::VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);
    worker_.CheckTimeouts();

    SUCCEED();
}

TEST_F(DaemonWorkerFixture, CheckTimeouts_OnWriteTimeBaseStatusFailure_Fails) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(0);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);
    score::time::VirtualLocalTime vlt_to_inject1(1000000);

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());
    auto writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(worker_.time_base_writers_[1].get());

    score::time::SynchronizationStatus status_to_inject =
        score::time::SynchronizationStatus::kSynchronized;  // global sync
    score::time::VirtualLocalTime vlt_to_inject2(0);

    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject1));
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<score::time::SynchronizationStatus&>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<score::time::VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject2), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);

    EXPECT_CALL(*writer_mock, lock()).Times(1);
    EXPECT_CALL(*writer_mock, Write(An<const score::time::SynchronizationStatus&>())).WillOnce(Return(false));
    EXPECT_CALL(*writer_mock, unlock()).Times(1);

    worker_.CheckTimeouts();

    SUCCEED();
}

TEST_F(DaemonWorkerFixture, CheckTimeouts_OnWriteTimeBaseStatusSuccess_Succeed) {
    // Arrange
    TimeBaseConfigData cfg_valid1{"TEST2", TsyncTimeDomainConfig{{}, {}, {}, {}, {}, 2}};

    // The negative timeout value forces the timeout check in TsyncWorker::CheckTimeouts to kick in
    cfg_valid1.timebase_config.sync_loss_timeout = std::chrono::milliseconds(0);
    // We require a valid consumer config for this test
    cfg_valid1.timebase_config.consumer_config.time_slave_config.is_valid = true;
    cfg_valid1.timebase_config.consumer_config.name = "consumer";
    TimeBaseConfiguration::GetInstance().AddConfigData(cfg_valid1);
    score::time::VirtualLocalTime vlt_to_inject1(1000000);

    // Act
    worker_.Init();
    EXPECT_TRUE(worker_.is_initialized_);

    auto reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(worker_.time_base_readers_[1].get());
    auto writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(worker_.time_base_writers_[1].get());
    ::testing::Mock::AllowLeak(reader_mock);
    ::testing::Mock::AllowLeak(writer_mock);

    score::time::SynchronizationStatus status_to_inject =
        score::time::SynchronizationStatus::kSynchronized;  // global sync
    score::time::VirtualLocalTime vlt_to_inject2(0);

    std::string_view reader_name("/reader_one");

    EXPECT_CALL(*shared_utils_mock.get(), GetCurrentVirtualLocalTime()).WillOnce(Return(vlt_to_inject1));
    EXPECT_CALL(*reader_mock, lock()).Times(2);
    EXPECT_CALL(*reader_mock, Read(An<score::time::SynchronizationStatus&>()))
        .WillOnce(DoAll(SetArgReferee<0>(status_to_inject), Return(true)));
    EXPECT_CALL(*reader_mock, Read(An<score::time::VirtualLocalTime&>()))
        .WillOnce(DoAll(SetArgReferee<0>(vlt_to_inject2), Return(true)));
    EXPECT_CALL(*reader_mock, unlock()).Times(2);

    EXPECT_CALL(*writer_mock, lock()).Times(1);
    EXPECT_CALL(*writer_mock, Write(An<const score::time::SynchronizationStatus&>())).WillOnce(Return(true));
    EXPECT_CALL(*writer_mock, unlock()).Times(1);

    worker_.CheckTimeouts();

    SUCCEED();
}

}  // namespace daemon_worker_ut
}  // namespace testing
