/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "tsync-ptp-lib/internal/tsync_data_broker.h"
#include "tsync-ptp-lib/tsync_ptp_lib.h"
#include "score/time/utility/TsyncConfigTypes.h"
#include "score/time/utility/SysCalls.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncNamedSemaphore.h"
#include "score/time/utility/TsyncSharedUtils.h"

using score::time::OsSemOpen;

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "matcher_operators.h"
#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseWriterMock.h"
#include "SysCallsMiscMock.h"
#include "SysCallsNamedSemMock.h"
#include "SysCallsReadWriteLockMock.h"
#include "SysCallsShMemMock.h"
#include "TimeBaseReaderFactoryMock.h"
#include "TimeBaseWriterFactoryMock.h"

using score::time::kIdMappingsShmemFileName;
using score::time::kIdMappingsShmemSize;
using score::time::kSharedMemPageSize;
using score::time::TsyncNamedSemaphore;

#define NUM_FAKE_TIMEBASE_ID_MAPPINGS 3

#define FAKE_TIMEBASE_ID_0 (static_cast<TSync_SynchronizedTimeBaseType>(1))
#define FAKE_TIMEBASE_ID_1 (static_cast<TSync_SynchronizedTimeBaseType>(2))
#define FAKE_TIMEBASE_ID_2 (static_cast<TSync_SynchronizedTimeBaseType>(3))
#define FAKE_INSTANCE_SPECIFIER_0 "fakeId0"
#define FAKE_INSTANCE_SPECIFIER_1 "fakeId1"
#define FAKE_INSTANCE_SPECIFIER_2 "/fakeId2"

constexpr int32_t kNumTimeDomains{16};
const uint16_t kTimeDomainIds[kNumTimeDomains] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static constexpr std::uint64_t kConsumerMagic_ = 0xabcddcbaabcddcba;
static constexpr std::uint64_t kProviderMagic_ = 0x1122334444332211;

const std::string_view kTimeDomainNames[kNumTimeDomains] = {
    "domain_1", "domain_2",  "domain_3",  "domain_4",  "domain_5",  "domain_6",  "domain_7",  "domain_8",
    "domain_9", "domain_10", "domain_11", "domain_12", "domain_13", "domain_14", "domain_15", "domain_16",
};

extern TSync_DB_Result tsync_db_OpenIdMappings(void);

void TSync_HandleCallBackThreadCleanup(TSync_TimeBaseHandleType timeBaseHandle);

extern TSync_DB_Timebase_Meta_Data tsync_db_timebase_meta_data[TSYNC_DB_MAX_TIMEBASES];

extern void* TSync_TransmitGlobalTimeRunner(void* arg);
extern TSync_DB_Timebase_Meta_Data* tsync_db_GetTimeBaseMetaDataElement(TSync_SynchronizedTimeBaseType timeBaseId);

using testing::_;
using testing::An;
using testing::ByMove;
using testing::DoAll;
using testing::DoDefault;
using testing::Eq;
using testing::HasSubstr;
using testing::Invoke;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnPointee;
using testing::Sequence;

namespace score {
namespace time {
std::unique_ptr<NiceMock<score::time::SysCallsReadWriteLockMock>> rw_lock_mock;
std::unique_ptr<NiceMock<score::time::SysCallsNamedSemMock>> named_semaphore_mock;
std::unique_ptr<NiceMock<score::time::SysCallsMiscMock>> misc_mock;
std::unique_ptr<NiceMock<score::time::TimeBaseWriterFactoryMock>> writer_factory_mock;
std::unique_ptr<NiceMock<score::time::TimeBaseReaderFactoryMock>> reader_factory_mock;
extern bool writer_factory_mock_return_real_writer;
extern bool reader_factory_mock_return_real_reader;
}  // namespace time
}  // namespace score

using score::time::ITimeBaseReader;
using score::time::ITimeBaseWriter;
using score::time::misc_mock;
using score::time::named_semaphore_mock;
using score::time::reader_factory_mock;
using score::time::reader_factory_mock_return_real_reader;
using score::time::rw_lock_mock;
using score::time::shared_mem_mock;
using score::time::SharedMemTimeBaseReaderMock;
using score::time::SharedMemTimeBaseWriterMock;
using score::time::SysCallsMiscMock;
using score::time::SysCallsNamedSemMock;
using score::time::SysCallsReadWriteLockMock;
using score::time::SysCallsShMemMock;
using score::time::TimeBaseReaderFactory;
using score::time::TimeBaseReaderFactoryMock;
using score::time::TimeBaseWriterFactory;
using score::time::TimeBaseWriterFactoryMock;
using score::time::TsyncCrcSupport;
using score::time::TsyncCrcValidation;
using score::time::TsyncEthMessageFormat;
using score::time::TsyncIdMappingsHandler;
using score::time::TsyncTimeDomainConfig;
using score::time::writer_factory_mock;
using score::time::writer_factory_mock_return_real_writer;

int CreateThreadFake(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
    return pthread_create(thread, attr, start_routine, arg);
}

sem_t* SemOpenFake1(const char* name, int oflag) {
    return sem_open(name, oflag);
}

sem_t* SemOpenFake2(const char* name, int oflag, mode_t mode, unsigned int value) {
    return sem_open(name, oflag, mode, value);
}

int SemCloseFake(sem_t* sem) {
    return sem_close(sem);
}

int SemUnlinkFake(const char* name) {
    return sem_unlink(name);
}

int SemWaitFake(sem_t* sem) {
    return sem_wait(sem);
}

int SemPostFake(sem_t* sem) {
    return sem_post(sem);
}

int SemTryWaitFake(sem_t* sem) {
    return sem_trywait(sem);
}

int SemGetValueFake(sem_t* sem, int* val) {
    return sem_getvalue(sem, val);
}

int ThreadJoinFake(pthread_t thread, void** retval) {
    return pthread_join(thread, retval);
}

constexpr int32_t ABORT_CODE = 42;

class PtpLibTestFixture : public testing::Test {
   public:
    void SetUp() override {
        std::memset(&shmem_buf0_[0], 0, sizeof(shmem_buf0_));
        std::memset(&shmem_buf1_[0], 0, sizeof(shmem_buf1_));
        std::memset(&shmem_buf2_[0], 0, sizeof(shmem_buf2_));
        std::memset(&shmem_buf_id_mappings[0], 0, sizeof(shmem_buf_id_mappings));

        // using real readers and writers, the syscalls are mocked
        reader_factory_mock_return_real_reader = true;
        writer_factory_mock_return_real_writer = true;

        // Create mocks
        writer_factory_mock = std::make_unique<NiceMock<TimeBaseWriterFactoryMock>>();
        reader_factory_mock = std::make_unique<NiceMock<TimeBaseReaderFactoryMock>>();
        named_semaphore_mock = std::make_unique<NiceMock<SysCallsNamedSemMock>>();
        shared_mem_mock = std::make_unique<NiceMock<SysCallsShMemMock>>();
        rw_lock_mock = std::make_unique<NiceMock<SysCallsReadWriteLockMock>>();
        misc_mock = std::make_unique<NiceMock<SysCallsMiscMock>>();

        // Setup default expectations for success cases
        // Named semaphore mock
        ON_CALL(*named_semaphore_mock, OsSemOpen(_, _)).WillByDefault(Invoke(SemOpenFake1));
        ON_CALL(*named_semaphore_mock, OsSemOpen(_, _, _, _)).WillByDefault(Invoke(SemOpenFake2));
        ON_CALL(*named_semaphore_mock, OsSemClose(_)).WillByDefault(Invoke(SemCloseFake));
        ON_CALL(*named_semaphore_mock, OsSemUnlink(_)).WillByDefault(Invoke(SemUnlinkFake));
        ON_CALL(*named_semaphore_mock, OsSemWait(_)).WillByDefault(Invoke(SemWaitFake));
        ON_CALL(*named_semaphore_mock, OsSemTryWait(_)).WillByDefault(Invoke(SemTryWaitFake));
        ON_CALL(*named_semaphore_mock, OsSemPost(_)).WillByDefault(Invoke(SemPostFake));
        ON_CALL(*named_semaphore_mock, OsSemGetValue(_, _)).WillByDefault(Invoke(SemGetValueFake));

        // Shared memory mock
        // different filenames return different file desriptors
        ON_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(kIdMappingsShmemFileName), _, _))
            .WillByDefault(Return(file_descriptor_map_[kIdMappingsShmemFileName]));
        ON_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(FAKE_INSTANCE_SPECIFIER_0), _, _))
            .WillByDefault(Return(file_descriptor_map_["/" FAKE_INSTANCE_SPECIFIER_0]));
        ON_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(FAKE_INSTANCE_SPECIFIER_1), _, _))
            .WillByDefault(Return(file_descriptor_map_["/" FAKE_INSTANCE_SPECIFIER_1]));
        ON_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(FAKE_INSTANCE_SPECIFIER_2), _, _))
            .WillByDefault(Return(file_descriptor_map_["/" FAKE_INSTANCE_SPECIFIER_2]));

        // different filedescriptors return different memory buffers
        ON_CALL(*shared_mem_mock, OsMmap(_, _, _, _, Eq(1), _)).WillByDefault(Return(timebase_shmem_map_[1]));
        ON_CALL(*shared_mem_mock, OsMmap(_, _, _, _, Eq(2), _)).WillByDefault(Return(timebase_shmem_map_[2]));
        ON_CALL(*shared_mem_mock, OsMmap(_, _, _, _, Eq(3), _)).WillByDefault(Return(timebase_shmem_map_[3]));
        ON_CALL(*shared_mem_mock, OsMmap(_, _, _, _, Eq(4), _)).WillByDefault(Return(timebase_shmem_map_[4]));

        ON_CALL(*shared_mem_mock, OsShmUnlink(_)).WillByDefault(Return(0));
        ON_CALL(*shared_mem_mock, OsFtruncate(_, _)).WillByDefault(Return(0));
        ON_CALL(*shared_mem_mock, OsClose(_)).WillByDefault(Return(0));

        // Read Write lock mock
        ON_CALL(*rw_lock_mock, OsRwLockTryReadLock(_)).WillByDefault(Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockReadLock(_)).WillByDefault(Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockTryWriteLock(_)).WillByDefault(Return(0));
        ON_CALL(*rw_lock_mock, OsRwLockUnlock(_)).WillByDefault(Return(0));

        // Time relevent mock
        ON_CALL(*misc_mock, OsClockGetTime(_, _)).WillByDefault(Return(0));

        // Thread relevent mock
        ON_CALL(*misc_mock, OsThreadCreate(_, _, _, _)).WillByDefault(Invoke(CreateThreadFake));
        ON_CALL(*misc_mock, OsThreadJoin(_, _)).WillByDefault(Invoke(ThreadJoinFake));

        Mock::AllowLeak(named_semaphore_mock.get());
        Mock::AllowLeak(misc_mock.get());

        ts_.timeBaseStatus = TIMEBASE_STATUS_BIT_GLOBAL_SYNC;
        ts_.seconds = 0x98765432;
        ts_.secondsHi = 0x9199;
        ts_.nanoseconds = 0xbbaaccdd;

        vt_.nanosecondsHi = 0x1234;
        vt_.nanosecondsLo = 0x5678;

        ud_.userDataLength = 3;
        ud_.userByte0 = 1;
        ud_.userByte1 = 2;
        ud_.userByte2 = 3;

        //!! ara::core::SetAbortHandler(&AbortHandler);
        //!! std::atexit(ExitHandler);

        // Create fake timebase id mappings
        TsyncTimeDomainConfig cfg;
        cfg.consumer_config.time_slave_config.is_valid = true;
        FillTimebaseIdMappings();
        for (auto it : id_mappings_map_) {
            WriteConfig(it.second, cfg);
        }

        InitSemaphores();
    }

    void TearDown() override {
        // As we use singleton mock objects, clear expectations after each test
        TSync_Close();
        DeleteSemaphores();
        mappings_handler_.Clear();
        ExitHandler();
    }

    static void ExitHandler() {
        // make sure to call Tsync_Close() before invalidating the syscall mocks
        // This will allow internal static objects to be cleaned off properly
        TSync_Close();
        // the mocks have to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        reader_factory_mock.reset();
        writer_factory_mock.reset();
        named_semaphore_mock.reset();
        rw_lock_mock.reset();
        shared_mem_mock.reset();
        misc_mock.reset();
    }

    static void AbortHandler() noexcept {
        // the mock has to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        ExitHandler();
        std::exit(ABORT_CODE);
    }

    void InvalidateTimeBaseMappings() {
        mappings_handler_.Clear();
        mappings_handler_.CommitMappingsToSharedMemory();
    }

    void InitSemaphores() {
        // tsyncd is usually the creator/owner of theses semaphores.
        // creating them here as part of the infrastructure setup
        for (uint16_t id = 0; id < kNumTimeDomains; ++id) {
            semaphores_.push_back(score::time::TsyncSharedUtils::CreateTransmissionSemaphore(id, true));
        }
    }

    void DeleteSemaphores() {
        semaphores_.clear();
    }

    TSync_TimeStampType ts_;
    TSync_VirtualLocalTimeType vt_;
    TSync_UserDataType ud_;
    TsyncIdMappingsHandler mappings_handler_;
    std::vector<TsyncNamedSemaphore> semaphores_;

    void FillTimebaseIdMappings(void) {
        for (auto it : id_mappings_map_) {
            mappings_handler_.AddDomainMapping(it.second, it.first);
        }

        mappings_handler_.CommitMappingsToSharedMemory();
        ASSERT_FALSE(mappings_handler_.IsEmpty());
    }

    static void WriteConfig(TSync_SynchronizedTimeBaseType timebase_id, TsyncTimeDomainConfig& cfg) {
        auto writer = TimeBaseWriterFactory::Create(
            std::find_if(id_mappings_map_.begin(), id_mappings_map_.end(),
                         [timebase_id](const auto& it) { return it.second == timebase_id; })
                ->first);
        writer->GetAccessor().Open();
        std::lock_guard<ITimeBaseWriter> lock(*writer);
        writer->Write(cfg);
    }

   private:
    static uint8_t shmem_buf0_[kSharedMemPageSize];
    static uint8_t shmem_buf1_[kSharedMemPageSize];
    static uint8_t shmem_buf2_[kSharedMemPageSize];
    static uint8_t shmem_buf_id_mappings[kIdMappingsShmemSize];

    // mapping of timebase instance specifiers to timebase ids
    using IdMappingsMap = std::map<std::string_view, TSync_SynchronizedTimeBaseType>;
    // mapping of shmem file names to file desriptors
    using FileDescriptorMap = std::map<std::string_view, int>;
    // mapping of shmem file descriptors to buffers
    using TimeBaseShMemMap = std::map<int, void*>;

    static IdMappingsMap id_mappings_map_;
    static FileDescriptorMap file_descriptor_map_;
    static TimeBaseShMemMap timebase_shmem_map_;
};

uint8_t PtpLibTestFixture::shmem_buf0_[kSharedMemPageSize];
uint8_t PtpLibTestFixture::shmem_buf1_[kSharedMemPageSize];
uint8_t PtpLibTestFixture::shmem_buf2_[kSharedMemPageSize];
uint8_t PtpLibTestFixture::shmem_buf_id_mappings[kIdMappingsShmemSize];

PtpLibTestFixture::IdMappingsMap PtpLibTestFixture::id_mappings_map_ = {
    {FAKE_INSTANCE_SPECIFIER_0, FAKE_TIMEBASE_ID_0},
    {FAKE_INSTANCE_SPECIFIER_1, FAKE_TIMEBASE_ID_1},
    {FAKE_INSTANCE_SPECIFIER_2, FAKE_TIMEBASE_ID_2}};

PtpLibTestFixture::FileDescriptorMap PtpLibTestFixture::file_descriptor_map_ = {{"/" FAKE_INSTANCE_SPECIFIER_0, 1},
                                                                                {"/" FAKE_INSTANCE_SPECIFIER_1, 2},
                                                                                {"/" FAKE_INSTANCE_SPECIFIER_2, 3},
                                                                                {kIdMappingsShmemFileName, 4}};

PtpLibTestFixture::TimeBaseShMemMap PtpLibTestFixture::timebase_shmem_map_ = {
    {1, &shmem_buf0_[0]}, {2, &shmem_buf1_[0]}, {3, &shmem_buf2_[0]}, {4, &shmem_buf_id_mappings[0]}};

namespace testing {
namespace lib_tsyncptplib_ut {

TEST_F(PtpLibTestFixture, Tsync_Open_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_Open_Twice_Succeeds) {
    // no arrange
    // act and assert (various steps)
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_CloseTimebase_AfterClose_Succeeds) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(handle, TSYNC_INVALID_HANDLE);
    TSync_Close();
    TSync_DB_Timebase_Meta_Data* md = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
    md->isInitialized = true;
    // act
    TSync_CloseTimebase(handle);
}

TEST_F(PtpLibTestFixture, TSync_CloseTimeBase_WithInvalidHandle_Succeeds) {
    // no arrange
    // act and assert (various steps)
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto handle = TSync_OpenTimebase(99);
    ASSERT_EQ(handle, TSYNC_INVALID_HANDLE);
    TSync_CloseTimebase(handle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_CloseTimeBase_WithoutIdMappings_Succeeds) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(handle, TSYNC_INVALID_HANDLE);
    // act
    TSync_Close();
    TSync_CloseTimebase(handle);

    // ASSERT
    SUCCEED();
}

TEST_F(PtpLibTestFixture, TSync_OpenTimeBase_WithoutOpenCall_Fails) {
    // no arrange
    // act and assert
    auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_EQ(TSYNC_INVALID_HANDLE, handle);
}

TEST_F(PtpLibTestFixture, TSync_OpenTimeBase_CalledTwice_Succeeds) {
    // no arrange
    // act and assert (various steps)
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

    auto handle2 = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_EQ(handle, handle2);

    TSync_CloseTimebase(handle);
    TSync_CloseTimebase(handle2);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_OpenTimebase_WithInitializedHandleMetadata_Succeed) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timebase_meta_data = tsync_db_GetTimeBaseMetaDataElement(FAKE_TIMEBASE_ID_0);
    timebase_meta_data->isInitialized.store(true);
    // act
    TSync_TimeBaseHandleType timeBaseHandle;
    std::thread open_tb_thread([&timeBaseHandle]() { timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0); });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    timebase_meta_data->isInitialized.store(false);
    open_tb_thread.join();
    // assert
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_OpenTimebase_WithInvalidTimeBaseId_Fails) {
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto h = TSync_OpenTimebase(99);
    ASSERT_EQ(h, TSYNC_INVALID_HANDLE);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, Tsync_OpenTimebaseWithInvalidTimebaseId_Fails) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(TSYNC_INVALID_TIME_BASE_ID);
    ASSERT_EQ(h, TSYNC_INVALID_HANDLE);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, Tsync_OpenTimebase_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    // need to inject a valid config for this to succeed
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);
    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, Tsync_OpenTimebase_ConsumerRole_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    // need to inject a valid config for this to succeed
    for (uint16_t i = 0; i < TSYNC_DB_MAX_TIMEBASES; ++i) {
        if (tsync_db_timebase_meta_data[i].mappingData.timeBaseId == FAKE_TIMEBASE_ID_0) {
            tsync_db_timebase_meta_data[i].mappingData.timeBaseRole = TSYNC_DB_TIMEBASE_ROLE_CONSUMER;
            break;
        }
    }
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);
    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, Tsync_OpenTimebase_WithoutSlash_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_1);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);
    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_SetTimeoutStatus_TimeoutBeforeSync_Succeeds) {
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    TSync_TimeStampType ts;
    TSync_VirtualLocalTimeType vt;
    TSync_UserDataType ud;

    result = TSync_SetTimeoutStatus(timeBaseHandle);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(timeBaseHandle, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    // Timeout shall not be set as the first sync has not happened yet
    TSync_TimeBaseStatusType status_expected = 0u;

    ASSERT_EQ(status_expected, ts.timeBaseStatus);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_WithInvalidHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    // act
    result = TSync_BusSetGlobalTime(TSYNC_INVALID_HANDLE, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_WithUnInitializedHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    TSync_DB_Timebase_Meta_Data* metaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    metaData->isInitialized.store(false);
    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_WithNullPtrParams_Succeeds) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_OnReadCurrentStatusFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    score::time::SynchronizationStatus status{};
    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::SynchronizationStatus&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(status), Return(false)));

    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_OnWriteTimeStampWithStatusFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    writer_factory_mock_return_real_writer = false;
    auto writer_mock = writer_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0, false);
    auto raw_writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(writer_mock.get());

    reader_factory_mock_return_real_reader = false;
    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    EXPECT_CALL(*writer_factory_mock, Create(_, _, _)).WillOnce(Return(ByMove(std::move(writer_mock))));

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    score::time::SynchronizationStatus status{score::time::SynchronizationStatus::kSynchronized};
    EXPECT_CALL(*raw_reader_mock, Read(testing::An<score::time::SynchronizationStatus&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(status), Return(true)));
    EXPECT_CALL(*raw_writer_mock, Write(testing::An<const score::time::TimestampWithStatus&>())).WillOnce(Return(false));

    // act
    TSync_TimeStampType ts;
    result = TSync_BusSetGlobalTime(timeBaseHandle, &ts, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_WithTimeStampSkipFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    writer_factory_mock_return_real_writer = false;

    auto writer_mock = writer_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0, false);
    auto raw_mock = static_cast<SharedMemTimeBaseWriterMock*>(writer_mock.get());

    EXPECT_CALL(*writer_factory_mock, Create(_, _, _)).WillOnce(Return(ByMove(std::move(writer_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(true));
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(false));

    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_WithLocalTimeSkipFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    writer_factory_mock_return_real_writer = false;

    auto writer_mock = writer_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0, false);
    auto raw_mock = static_cast<SharedMemTimeBaseWriterMock*>(writer_mock.get());

    EXPECT_CALL(*writer_factory_mock, Create(_, _, _)).WillOnce(Return(ByMove(std::move(writer_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(true));
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(false));

    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTime_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    result = TSync_BusSetGlobalTime(h, &ts_, nullptr, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 2;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 1;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 0;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 3;

    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusSetGlobalTimeWithSynctoGateWayStatus_Succeeds) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::SynchronizationStatus&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(score::time::SynchronizationStatus::kSynchToGateway),
                                   Return(true)));

    // act
    result = TSync_BusSetGlobalTime(timeBaseHandle, &ts_, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_OK);

    // clean-up
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithInvalidHandle_Fails) {
    // no arrange
    // act
    TSync_ReturnType result = TSync_BusGetGlobalTime(TSYNC_INVALID_HANDLE, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithUnInitializedHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    TSync_DB_Timebase_Meta_Data* metaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    metaData->isInitialized.store(false);
    // act
    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithNullPtrParams_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(result, E_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithTimeStampSkipFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(true));
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(false));

    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithLocalTimeSkipFailure_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(true));
    EXPECT_CALL(*raw_mock, SetPosition(_)).WillOnce(Return(false));

    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithTimeStampReadReturnFalse_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    TSync_TimeStampType ts;

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;
    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::TimestampWithStatus&>())).WillOnce(Return(false));

    result = TSync_BusGetGlobalTime(timeBaseHandle, &ts, nullptr, nullptr, nullptr);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithVirtualLocalTimeReadReturnFalse_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    TSync_VirtualLocalTimeType vlt;

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;

    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::VirtualLocalTime&>())).WillOnce(Return(false));

    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, nullptr, nullptr, &vlt);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithLUserDataReadReturnFalse_Fails) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    TSync_UserDataType ud;

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;

    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::UserDataView&>())).WillOnce(Return(false));

    result = TSync_BusGetGlobalTime(timeBaseHandle, nullptr, &ud, nullptr, nullptr);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_Succeeds) {
    // no arrange
    reader_factory_mock_return_real_reader = true;
    writer_factory_mock_return_real_writer = true;
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    TSync_TimeStampType ts;
    TSync_VirtualLocalTimeType vt;
    TSync_UserDataType ud;

    ud.userDataLength = 2;

    // Set the time values
    ts.nanoseconds = 11;
    ts.seconds = 22;
    ts.secondsHi = 33;
    ts.timeBaseStatus = 0;

    // act and assert (various steps)
    ud_.userDataLength = 0;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(h, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 1;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(h, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 2;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(h, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    ud_.userDataLength = 3;
    result = TSync_BusSetGlobalTime(h, &ts_, &ud_, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(h, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    // Sync status after first TSync_BusSetGlobalTime must be 8
    TSync_TimeBaseStatusType status_expected = 1 << TIMEBASE_STATUS_BIT_GLOBAL_SYNC;

    EXPECT_EQ(status_expected, ts.timeBaseStatus);
    EXPECT_EQ(ts.seconds, ts_.seconds);
    EXPECT_EQ(ts.secondsHi, ts_.secondsHi);
    EXPECT_EQ(ts.nanoseconds, ts_.nanoseconds);
    EXPECT_EQ(vt.nanosecondsHi, vt_.nanosecondsHi);
    EXPECT_EQ(vt.nanosecondsLo, vt_.nanosecondsLo);
    EXPECT_EQ(ud.userDataLength, ud_.userDataLength);
    EXPECT_EQ(ud.userByte0, ud_.userByte0);
    EXPECT_EQ(ud.userByte1, ud_.userByte1);
    EXPECT_EQ(ud.userByte2, ud_.userByte2);

    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_BusGetGlobalTime_WithSynctoGatewayStatus_Succeeds) {
    // arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    score::time::TimestampWithStatus ts_read{score::time::SynchronizationStatus::kSynchToGateway,
                                            std::chrono::nanoseconds(123), std::chrono::seconds(456)};

    TSync_TimeStampType ts;

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    testing::InSequence seq;

    EXPECT_CALL(*raw_mock, Read(testing::An<score::time::TimestampWithStatus&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(ts_read), Return(true)));

    result = TSync_BusGetGlobalTime(timeBaseHandle, &ts, nullptr, nullptr, nullptr);
    ASSERT_EQ(result, E_OK);

    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetCurrentVirtualLocalTime_WithInvalidHandle_Fails) {
    // no arrange
    // act
    TSync_ReturnType result = TSync_GetCurrentVirtualLocalTime(TSYNC_INVALID_HANDLE, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);
}

TEST_F(PtpLibTestFixture, TSync_GetCurrentVirtualLocalTime_WithUnInitializedHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    TSync_DB_Timebase_Meta_Data* metaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    metaData->isInitialized.store(false);
    // act
    TSync_VirtualLocalTimeType vt;
    result = TSync_GetCurrentVirtualLocalTime(timeBaseHandle, &vt);
    // assert
    ASSERT_EQ(result, E_NOT_OK);
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetCurrentVirtualLocalTime_OnClockGettimeError_Fails) {
    // arrange
    EXPECT_CALL(*misc_mock, OsClockGetTime(_, _)).WillOnce(Return(-1));
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    TSync_VirtualLocalTimeType vt;

    result = TSync_GetCurrentVirtualLocalTime(h, &vt);
    ASSERT_EQ(result, E_NOT_OK);

    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetCurrentVirtualLocalTime_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    TSync_VirtualLocalTimeType vt;

    result = TSync_GetCurrentVirtualLocalTime(h, &vt);
    ASSERT_EQ(result, E_OK);

    TSync_CloseTimebase(h);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetCurrentVirtualLocalTime_WithMultipleTimebase_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_TimeBaseHandleType h2 = TSync_OpenTimebase(FAKE_TIMEBASE_ID_1);
    ASSERT_NE(h2, TSYNC_INVALID_HANDLE);

    TSync_VirtualLocalTimeType vt;
    TSync_VirtualLocalTimeType vt2;

    result = TSync_GetCurrentVirtualLocalTime(h, &vt);
    ASSERT_EQ(result, E_OK);

    result = TSync_GetCurrentVirtualLocalTime(h2, &vt2);
    ASSERT_EQ(result, E_OK);

    TSync_CloseTimebase(h);
    TSync_Close();

    TSync_CloseTimebase(h2);
    TSync_Close();
}

TSync_ReturnType ptpds_TransmitGlobalTime(TSync_SynchronizedTimeBaseType tb) {
    (void)tb;
    printf("\n%s\n", __func__);
    return E_OK;
}

TEST_F(PtpLibTestFixture, TSync_SetTimeoutStatus_TimeoutAfterSync_Succeeds) {
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

    TSync_TimeStampType ts;
    TSync_VirtualLocalTimeType vt;
    TSync_UserDataType ud;

    // Call TSync_BusSetGlobalTime before setting timeout bit
    result = TSync_BusSetGlobalTime(timeBaseHandle, &ts_, nullptr, nullptr, &vt_);
    ASSERT_EQ(result, E_OK);

    result = TSync_SetTimeoutStatus(timeBaseHandle);
    ASSERT_EQ(result, E_OK);

    result = TSync_BusGetGlobalTime(timeBaseHandle, &ts, &ud, nullptr, &vt);
    ASSERT_EQ(result, E_OK);

    // Status has 2 bits set - Timeout bit and Sync bit - 0000 1001
    TSync_TimeBaseStatusType status_expected = (1 << TIMEBASE_STATUS_BIT_TIMEOUT);

    ASSERT_EQ(status_expected, ts.timeBaseStatus);
}

TEST_F(PtpLibTestFixture, TSync_SetTimeoutStatus_WithInvalidhandle_Fails) {
    TSync_ReturnType result = TSync_SetTimeoutStatus(TSYNC_INVALID_HANDLE);
    ASSERT_EQ(result, E_NOT_OK);
}

TEST_F(PtpLibTestFixture, TSync_TransmitGlobalTimeRunner_WithUnInitializedHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    TSync_DB_Timebase_Meta_Data* metaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    metaData->isInitialized.store(false);
    // act
    auto resultPtr = TSync_TransmitGlobalTimeRunner(timeBaseHandle);
    // assert
    ASSERT_EQ(resultPtr, nullptr);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_TransmitGlobalTimeRunner_WithInvalidArguments_Fails) {
    // arrange
    ASSERT_EXIT(
        {
            // act
            TSync_TransmitGlobalTimeRunner(nullptr);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

std::mutex transmission_mutex;
int ptpds_TransmitGlobalTimeCounter = 0;
TSync_ReturnType ptpds_TransmitGlobalTimeCounterFunction(TSync_SynchronizedTimeBaseType tb) {
    (void)tb;
    std::lock_guard<std::mutex> lock(transmission_mutex);
    ++ptpds_TransmitGlobalTimeCounter;
    return E_OK;
}

TEST_F(PtpLibTestFixture, TSync_RegisterTransmitGlobalTimeCallback_Succeeds) {
    // no arrange
    // act and assert (various steps)
    auto result = TSync_Open();

    ASSERT_EQ(result, E_OK);
    auto h = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(h, TSYNC_INVALID_HANDLE);

    result = TSync_RegisterTransmitGlobalTimeCallback(h, ptpds_TransmitGlobalTime);
    ASSERT_EQ(result, E_OK);

    result = TSync_RegisterTransmitGlobalTimeCallback(h, ptpds_TransmitGlobalTime);
    ASSERT_EQ(result, E_OK);

    // compensates the time that transmitGlobalTimeRunner needs for spawning the transmitGlobalTimeRunner thread
    // ptpd should be running and register the callback before a provider application
    // triggers a transmission on the bus
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // signal transmission to wake the thread and trigger the callback
    semaphores_[FAKE_TIMEBASE_ID_0].unlock();

    // give the transmission thread a litte time to trigger the callback
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    result = TSync_RegisterTransmitGlobalTimeCallback(h, nullptr);
    ASSERT_EQ(result, E_OK);

    result = TSync_RegisterTransmitGlobalTimeCallback(h, nullptr);  // unregistering a callback twice is not possible
    ASSERT_EQ(result, E_OK);

    TSync_CloseTimebase(h);

    // trigger thread cleanup with invalid handle for coverage
    TSync_HandleCallBackThreadCleanup(TSYNC_INVALID_HANDLE);

    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_SetTimeoutStatus_WithUnInitializedHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    auto timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    TSync_DB_Timebase_Meta_Data* metaData = static_cast<TSync_DB_Timebase_Meta_Data*>(timeBaseHandle);
    metaData->isInitialized.store(false);
    // act
    result = TSync_SetTimeoutStatus(timeBaseHandle);
    // assert
    ASSERT_EQ(result, E_NOT_OK);
    TSync_Close();
}

TSync_ReturnType Tsync_Test_TransmissionCallback(TSync_SynchronizedTimeBaseType) {
    return E_OK;
}

TEST_F(PtpLibTestFixture, TSync_CloseTwice_Succeeds) {
    // no arrange
    // act and assert (various steps)
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    TSync_Close();
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetTimebaseConfiguration_Succeeds) {
    // arrange: open lib and timebase
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    // use mocked reader so we can inject arbitrary configurations
    TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
    ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);
    // arrange: config (this will happen multiple times to get coverage for all good
    // code paths)
    TsyncTimeDomainConfig cfg;
    TSync_Role role_requested;

    role_requested = TSYNC_ROLE_MASTER;
    cfg.consumer_config.time_slave_config.is_valid = true;
    cfg.provider_config.time_master_config.is_valid = true;
    cfg.provider_config.time_master_config.sub_tlv_config.user_data_enabled = true;
    cfg.provider_config.time_master_config.sub_tlv_config.status_enabled = true;
    cfg.provider_config.time_master_config.sub_tlv_config.time_enabled = true;
    cfg.provider_config.time_master_config.crc_support = TsyncCrcSupport::kSupported;
    cfg.consumer_config.time_slave_config.crc_validation = TsyncCrcValidation::kIgnored;
    cfg.eth_time_domain.message_format = TsyncEthMessageFormat::kIeee8021AS;
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);

    // act: read config (repeated with different configs)
    TSync_TimeBaseConfiguration config;
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(result, E_OK);
    ASSERT_EQ(config.crcSupport, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.correction_field, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.domain_number, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.message_length, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.precise_origin_timestamp, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.sequence_id, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.source_port_identity, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcValidation, TSYNC_CRC_IGNORED);
    ASSERT_EQ(config.messageCompliance, TSYNC_MC_IEEE8021AS);
    ASSERT_EQ(config.role, TSYNC_ROLE_MASTER);
    ASSERT_EQ(config.subTlvConfig.followUpUserDataSubTLV, TSYNC_SUBTLV_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpStatusSubTLV, TSYNC_SUBTLV_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpTimeSubTLV, TSYNC_SUBTLV_SUPPORTED);
    // arrange
    cfg.provider_config.time_master_config.crc_support = TsyncCrcSupport::kNotSupported;
    cfg.eth_time_domain.crcFlags.correction_field = TSYNC_CRC_SUPPORTED;
    cfg.eth_time_domain.crcFlags.domain_number = TSYNC_CRC_SUPPORTED;
    cfg.eth_time_domain.crcFlags.message_length = TSYNC_CRC_SUPPORTED;
    cfg.eth_time_domain.crcFlags.precise_origin_timestamp = TSYNC_CRC_SUPPORTED;
    cfg.eth_time_domain.crcFlags.sequence_id = TSYNC_CRC_SUPPORTED;
    cfg.eth_time_domain.crcFlags.source_port_identity = TSYNC_CRC_SUPPORTED;
    cfg.consumer_config.time_slave_config.crc_validation = TsyncCrcValidation::kNotValidated;
    cfg.provider_config.time_master_config.sub_tlv_config.user_data_enabled = false;
    cfg.provider_config.time_master_config.sub_tlv_config.status_enabled = false;
    cfg.provider_config.time_master_config.sub_tlv_config.time_enabled = false;
    cfg.eth_time_domain.message_format = TsyncEthMessageFormat::kIeee8021ASAutosar;
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);

    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(result, E_OK);
    ASSERT_EQ(config.crcSupport, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcFlags.correction_field, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.domain_number, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.message_length, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.precise_origin_timestamp, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.sequence_id, TSYNC_CRC_SUPPORTED);
    ASSERT_EQ(config.crcFlags.source_port_identity, TSYNC_CRC_SUPPORTED);

    ASSERT_EQ(config.crcValidation, TSYNC_CRC_NOT_VALIDATED);
    ASSERT_EQ(config.messageCompliance, TSYNC_MC_IEEE8021AS_AUTOSAR);
    ASSERT_EQ(config.subTlvConfig.followUpUserDataSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpStatusSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpTimeSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);

    // arrange
    cfg.provider_config.time_master_config.crc_support = static_cast<TsyncCrcSupport>(13);  // invalid
    cfg.consumer_config.time_slave_config.crc_validation = TsyncCrcValidation::kOptional;
    cfg.eth_time_domain.message_format = static_cast<TsyncEthMessageFormat>(13);  // invalid
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(result, E_OK);
    ASSERT_EQ(config.crcSupport, TSYNC_CRC_NOT_SUPPORTED);
    ASSERT_EQ(config.crcValidation, TSYNC_CRC_OPTIONAL);
    ASSERT_EQ(config.messageCompliance, TSYNC_MC_IEEE8021AS_AUTOSAR);
    // arrange
    cfg.consumer_config.time_slave_config.crc_validation = TsyncCrcValidation::kValidated;
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(config.crcValidation, TSYNC_CRC_VALIDATED);
    // arrange
    cfg.consumer_config.time_slave_config.crc_validation = static_cast<TsyncCrcValidation>(13);  // invalid
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(config.crcValidation, TSYNC_CRC_OPTIONAL);
    // arrange
    cfg.eth_time_domain.num_fup_data_id_entries = TSYNC_MAX_FUP_DATA_ID_ENTRIES;
    for (int32_t i = 0; i < TSYNC_MAX_FUP_DATA_ID_ENTRIES; ++i) {
        cfg.eth_time_domain.fup_data_id_list[i] = static_cast<uint8_t>(i);
    }
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(config.numFupDataIdEntries, cfg.eth_time_domain.num_fup_data_id_entries);
    for (int32_t i = 0; i < TSYNC_MAX_FUP_DATA_ID_ENTRIES; ++i) {
        ASSERT_EQ(config.fupDataIdList[i], cfg.eth_time_domain.fup_data_id_list[i]);
    }
    // arrange
    role_requested = TSYNC_ROLE_SLAVE;
    cfg.provider_config.time_master_config.is_valid = false;
    cfg.consumer_config.time_slave_config.sub_tlv_config.status_enabled = true;
    cfg.consumer_config.time_slave_config.sub_tlv_config.time_enabled = true;
    cfg.consumer_config.time_slave_config.sub_tlv_config.user_data_enabled = true;
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(config.role, TSYNC_ROLE_SLAVE);
    ASSERT_EQ(config.subTlvConfig.followUpStatusSubTLV, TSYNC_SUBTLV_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpTimeSubTLV, TSYNC_SUBTLV_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpUserDataSubTLV, TSYNC_SUBTLV_SUPPORTED);

    // arrange
    cfg.consumer_config.time_slave_config.sub_tlv_config.status_enabled = false;
    cfg.consumer_config.time_slave_config.sub_tlv_config.time_enabled = false;
    cfg.consumer_config.time_slave_config.sub_tlv_config.user_data_enabled = false;
    WriteConfig(FAKE_TIMEBASE_ID_0, cfg);
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    // assert
    ASSERT_EQ(config.role, TSYNC_ROLE_SLAVE);
    ASSERT_EQ(config.subTlvConfig.followUpStatusSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpTimeSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpUserDataSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);

    // arrange
    role_requested = TSYNC_ROLE_INVALID;
    // act
    result = TSync_GetTimebaseConfiguration(timeBaseHandle, role_requested, &config);
    ASSERT_EQ(result, E_OK);
    ASSERT_EQ(config.subTlvConfig.followUpStatusSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpTimeSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);
    ASSERT_EQ(config.subTlvConfig.followUpUserDataSubTLV, TSYNC_SUBTLV_NOT_SUPPORTED);

    // cleanup
    TSync_CloseTimebase(timeBaseHandle);
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_GetTimebaseConfigurationWithInvalidHandle_Fails) {
    // arrange
    TSync_ReturnType result = TSync_Open();
    ASSERT_EQ(E_OK, result);
    // act
    result = TSync_GetTimebaseConfiguration(TSYNC_INVALID_HANDLE, TSYNC_ROLE_MASTER, nullptr);
    // assert
    ASSERT_EQ(result, E_NOT_OK);
    // cleanup
    TSync_Close();
}

TEST_F(PtpLibTestFixture, TSync_Open_With16ElementsInMapping_OK) {
    // arrange

    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    uint32_t num_zero{0};

    EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
    EXPECT_CALL(*raw_mock, Open()).Times(1);
    EXPECT_CALL(*raw_mock, lock()).Times(1);
    EXPECT_CALL(*raw_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kNumTimeDomains), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[0]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[1]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[2]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[3]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[4]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[5]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[6]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[7]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[8]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[9]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[10]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[11]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[12]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[13]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[14]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainIds[15]), Return(true)))
        .WillRepeatedly(::testing::DoAll(testing::SetArgReferee<0>(num_zero), Return(true)));
    EXPECT_CALL(*raw_mock, Read(testing::An<std::string_view&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[0]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[1]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[2]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[3]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[4]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[5]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[6]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[7]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[8]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[9]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[10]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[11]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[12]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[13]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[14]), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[15]), Return(true)));

    EXPECT_CALL(*raw_mock, Read(testing::An<uint64_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kConsumerMagic_), Return(true)))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kProviderMagic_), Return(true)));

    EXPECT_CALL(*raw_mock, unlock()).Times(1);

    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
}

TEST_F(PtpLibTestFixture, TSync_HandleCallBackThreadCleanup_threadStatusIsDead_Succeeds) {
    // Arrange
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);
    TSync_DB_Timebase_Meta_Data timeBaseMeta;
    timeBaseMeta.transmitGlobalTime.threadStatus.store(TSYNC_DB_THREAD_STATUS_DEAD);
    TSync_TimeBaseHandleType timeBaseHandle = static_cast<void*>(&timeBaseMeta);

    // Act
    TSync_HandleCallBackThreadCleanup(timeBaseHandle);
    TSync_Close();
}

using PtpLibDeathTestFixture = PtpLibTestFixture;

TEST_F(PtpLibDeathTestFixture, TSync_Open_WithoutIDMappings_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            InvalidateTimeBaseMappings();
            // act
            TSync_Open();
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, Tsync_Open_SharedMemOpenFails_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*shared_mem_mock, OsShmOpen(_, _, _)).WillOnce(Return(-1));
            // act
            (void)TSync_Open();
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, Tsync_Open_MmapUnavailable_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*shared_mem_mock, OsMmap(_, _, _, _, _, _)).WillRepeatedly(Return(MAP_FAILED));
            // act
            (void)TSync_Open();
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, TSync_OpenTimeBase_OnConfigReadError_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);

            reader_factory_mock_return_real_reader = false;

            auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
            auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

            EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
            EXPECT_CALL(*raw_mock, Read(An<TsyncTimeDomainConfig&>())).WillOnce(Return(false));

            // act
            (void)TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("could not read config of time domain"));
}

TEST_F(PtpLibDeathTestFixture, TSync_CloseTimeBase_WithTransmissionThreadNotDead_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

            // act
            TSync_DB_Timebase_Meta_Data* md = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
            md->transmitGlobalTime.threadStatus.store(TSYNC_DB_THREAD_STATUS_ALIVE);
            tsync_db_CloseTimebase(md);
        },
        ::testing::ExitedWithCode(ABORT_CODE),
        HasSubstr("transmitGlobalTime.threadStatus != TSYNC_DB_THREAD_STATUS_DEAD"));
}

TEST_F(PtpLibDeathTestFixture, TSync_CloseTimeBase_WithTransmissionThreadSignalNotNone_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

            // act
            TSync_DB_Timebase_Meta_Data* md = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
            md->transmitGlobalTime.threadSignal.store(TSYNC_DB_THREAD_SIGNAL_CONTINUE);
            tsync_db_CloseTimebase(md);
        },
        ::testing::ExitedWithCode(ABORT_CODE),
        HasSubstr("failed transmitGlobalTime.threadSignal != TSYNC_DB_THREAD_SIGNAL_NONE"));
}

TEST_F(PtpLibDeathTestFixture, TSync_CloseTimeBase_WithGetDomainNameFails_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

            // act
            TSync_DB_Timebase_Meta_Data* md = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
            md->mappingData.timeBaseId = 0xff;
            md->transmitGlobalTime.threadSignal.store(TSYNC_DB_THREAD_SIGNAL_CONTINUE);

            tsync_db_CloseTimebase(md);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("could not determine domain name for timebase"));
}

TEST_F(PtpLibDeathTestFixture, TSync_CloseTimeBase_WithTransmissionCallbackNotNull_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

            // act
            TSync_DB_Timebase_Meta_Data* md = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
            md->transmitGlobalTime.callback = &Tsync_Test_TransmissionCallback;
            tsync_db_CloseTimebase(md);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("callback != nullptr"));
}

TEST_F(PtpLibDeathTestFixture, TSync_RegisterTransmitGlobalTimeCallback_OnSemPostFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*named_semaphore_mock, OsSemPost(_)).WillOnce(Return(-1));

            // act and assert
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

            result = TSync_RegisterTransmitGlobalTimeCallback(timeBaseHandle, ptpds_TransmitGlobalTime);
            ASSERT_EQ(result, E_OK);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            TSync_CloseTimebase(timeBaseHandle);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, TSync_RegisterTransmitGlobalTimeCallback_OnThreadCreateFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*misc_mock, OsThreadCreate(_, _, _, _)).WillOnce(Return(-1));

            // act and assert
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

            result = TSync_RegisterTransmitGlobalTimeCallback(timeBaseHandle, ptpds_TransmitGlobalTime);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, TSync_RegisterTransmitGlobalTimeCallback_OnThreadJoinFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*misc_mock, OsThreadJoin(_, _)).WillOnce(Return(-1));

            // act and assert
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            TSync_TimeBaseHandleType timeBaseHandle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(timeBaseHandle, TSYNC_INVALID_HANDLE);

            result = TSync_RegisterTransmitGlobalTimeCallback(timeBaseHandle, ptpds_TransmitGlobalTime);
            ASSERT_EQ(result, E_OK);

            TSync_CloseTimebase(timeBaseHandle);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, TSync_OpenTimebase_OnInvalidTimeBaseReader_Aborts) {
    ASSERT_EXIT(
        {
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            // use fake reader factory mock to allow the creation of the reader to fail.
            reader_factory_mock_return_real_reader = false;
            EXPECT_CALL(*reader_factory_mock, Create(_, _))
                .WillOnce(Return(ByMove(std::unique_ptr<score::time::ITimeBaseReader>(nullptr))));
            (void)TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("could not open reader for time domain"));
}

TEST_F(PtpLibDeathTestFixture, TSync_OpenTimebase_OnInvalidConfig_Aborts) {
    ASSERT_EXIT(
        {
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            TsyncTimeDomainConfig cfg;
            cfg.consumer_config.time_slave_config.is_valid = false;
            cfg.provider_config.time_master_config.is_valid = false;
            // use fake reader to return fake config
            reader_factory_mock_return_real_reader = false;
            auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
            auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());
            EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
            EXPECT_CALL(*raw_mock, Read(An<TsyncTimeDomainConfig&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(cfg), Return(true)));

            (void)TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("has neither a valid provider nor consumer configuration"));
}

TEST_F(PtpLibDeathTestFixture, Tsync_OpenTimebase_OsShmOpenFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(kIdMappingsShmemFileName), _, _))
                .WillOnce(::testing::DoDefault());
            EXPECT_CALL(*shared_mem_mock, OsShmOpen(HasSubstr(FAKE_INSTANCE_SPECIFIER_0), _, _)).WillOnce(Return(-1));

            // act and assert (various steps)
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            (void)TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, Tsync_OpenTimebase_OsMmapFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*shared_mem_mock, OsMmap(_, _, _, _, _, _))
                .WillOnce(::testing::DoDefault())
                .WillOnce(Return(MAP_FAILED));
            // act and assert (various steps)
            TSync_ReturnType result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            (void)TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
        },
        ::testing::ExitedWithCode(ABORT_CODE), ".*");
}

TEST_F(PtpLibDeathTestFixture, TSync_GetTimebaseConfiguration_OnReadDomainConfigError_Aborts) {
    auto result = TSync_Open();
    ASSERT_EQ(result, E_OK);

    // use mocked reader
    reader_factory_mock_return_real_reader = false;

    auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
    auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());

    ASSERT_EXIT(
        {
            // arrange
            EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);
            EXPECT_CALL(*raw_mock, Read(An<TsyncTimeDomainConfig&>())).WillOnce(Return(false));

            // act
            TSync_TimeBaseConfiguration cfg;
            TSync_Role role_requested = TSYNC_ROLE_MASTER;  // Any role for now
            (void)TSync_GetTimebaseConfiguration(handle, role_requested, &cfg);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("Could not read time domain configuration"));
}

TEST_F(PtpLibDeathTestFixture, TSync_SetTimeoutStatus_OnTimeStampReadError_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);

            // use mocked reader
            reader_factory_mock_return_real_reader = false;

            auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
            auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());
            EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);
            EXPECT_CALL(*raw_mock, Read(An<score::time::TimestampWithStatus&>())).WillOnce(Return(false));
            (void)TSync_SetTimeoutStatus(handle);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("could not read timestamp"));
}

TEST_F(PtpLibDeathTestFixture, TSync_GetTimebaseConfiguration_OnInvalidConfig_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);

            // use mocked reader
            reader_factory_mock_return_real_reader = false;

            auto reader_mock = reader_factory_mock->Create(FAKE_INSTANCE_SPECIFIER_0);
            auto raw_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock.get());
            EXPECT_CALL(*reader_factory_mock, Create(_, _)).WillOnce(Return(ByMove(std::move(reader_mock))));
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);

            TsyncTimeDomainConfig domain_cfg;

            domain_cfg.consumer_config.time_slave_config.is_valid = false;
            domain_cfg.provider_config.time_master_config.is_valid = false;

            EXPECT_CALL(*raw_mock, Read(An<TsyncTimeDomainConfig&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_cfg), Return(true)));

            // act
            TSync_TimeBaseConfiguration cfg;
            TSync_Role role_requested = TSYNC_ROLE_MASTER;  // Any role for now
            (void)TSync_GetTimebaseConfiguration(handle, role_requested, &cfg);
        },
        ::testing::ExitedWithCode(ABORT_CODE),
        HasSubstr("Neither the time-consumer config nor the time-provider config is valid"));
}

TEST_F(PtpLibDeathTestFixture, TSync_CloseTimebase_OnNegativeRefCount_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto result = TSync_Open();
            ASSERT_EQ(result, E_OK);
            auto handle = TSync_OpenTimebase(FAKE_TIMEBASE_ID_0);
            ASSERT_NE(handle, TSYNC_INVALID_HANDLE);
            TSync_DB_Timebase_Meta_Data* meta_data = static_cast<TSync_DB_Timebase_Meta_Data*>(handle);
            meta_data->refCounter = 0;
            // act
            TSync_CloseTimebase(handle);
        },
        ::testing::ExitedWithCode(ABORT_CODE), HasSubstr("tsync_db_CloseTimebase -  refCounter is negative"));
}

}  // namespace lib_tsyncptplib_ut
}  // namespace testing
