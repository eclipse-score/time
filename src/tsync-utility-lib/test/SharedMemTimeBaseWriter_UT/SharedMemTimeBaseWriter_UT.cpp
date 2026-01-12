/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#include <array>

#define private public
#include "SharedMemTimeBaseWriter.h"
#undef private
//!!#include "ara/core/abort.h"
#include "score/span.hpp"

#include "score/time/utility/TsyncConfigTypes.h"
#include "matcher_operators.h"
#include "SysCallsReadWriteLockMock.h"
#include "SysCallsShMemMock.h"
#include "SharedMemLayout.h"
#include "SharedMemTimeBaseReader.h"

using score::cpp::span;

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

using namespace score::time;

class SharedMemTimeBaseWriterTestFixture : public ::testing::Test {
   public:
    static const int32_t EXIT_CODE;

   protected:
    void SetUpWriter(const std::string& name) {
        shared_mem_name_ = name;
        tb_writer_ = std::make_unique<SharedMemTimeBaseWriter>(shared_mem_name_);
    }

    void SetUp() override {
        tb_writer_ = std::make_unique<SharedMemTimeBaseWriter>(shared_mem_name_);
        shared_mem_mock = std::make_unique<::testing::NiceMock<SysCallsShMemMock>>();
        rw_lock_mock = std::make_unique<testing::NiceMock<SysCallsReadWriteLockMock>>();

        // Setup default expectations for success cases
        ON_CALL(*shared_mem_mock, OsShmOpen(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(32768));
        ON_CALL(*shared_mem_mock, OsFtruncate(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
        ON_CALL(*shared_mem_mock,
                OsMmap(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Return(mem_region_));
        ON_CALL(*shared_mem_mock, OsClose(::testing::_)).WillByDefault(::testing::Return(0));

        // install abort handler for our death tests
        //!! ara::core::SetAbortHandler(&AbortHandler);
        // As we use here singleton mock object, clear expectations after each test
        ::testing::Mock::AllowLeak(shared_mem_mock.get());
    }

    void TearDown() override {
        tb_writer_.reset();
        shared_mem_mock.reset();
        rw_lock_mock.reset();

        //!! ara::core::SetAbortHandler(nullptr);
    }

    static void AbortHandler() noexcept {
        // the mock has to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        shared_mem_mock.reset();
        rw_lock_mock.reset();
        std::exit(EXIT_CODE);
    }

    std::string shared_mem_name_ = "/test";
    uint8_t mem_region_[4096];
    std::unique_ptr<SharedMemTimeBaseWriter> tb_writer_;
};

const int32_t SharedMemTimeBaseWriterTestFixture::EXIT_CODE = 42;

template <class Type>
class SharedMemTimeBaseWriterComplexWriteTestFixture : public SharedMemTimeBaseWriterTestFixture {
   protected:
    Type struct_;
    Type* mem_ = reinterpret_cast<Type*>(reinterpret_cast<uint8_t*>(&this->mem_region_) + sizeof(pthread_rwlock_t));
};

template <class Type>
class SharedMemTimeBaseWriterConfigWriteTestFixture : public SharedMemTimeBaseWriterTestFixture {
   protected:
    Type written_struct_;
    Type read_struct_;
};

template <class Type>
class SharedMemTimeBaseWriterWriteTestFixture : public SharedMemTimeBaseWriterTestFixture {
   protected:
    Type value_ = 42;
    Type* mem_ = reinterpret_cast<Type*>(reinterpret_cast<uint8_t*>(&this->mem_region_) +
                                         sizeof(score::time::TsyncReadWriteLock));
};

using WriteTypes =
    ::testing::Types<float, double, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;
TYPED_TEST_SUITE(SharedMemTimeBaseWriterWriteTestFixture, WriteTypes);

using WriteComplexTypes = ::testing::Types<TimestampWithStatus, SynchronizationStatus>;
TYPED_TEST_SUITE(SharedMemTimeBaseWriterComplexWriteTestFixture, WriteComplexTypes);

using WriteConfigTypes = ::testing::Types<TsyncTimeDomainConfig, TsyncConsumerConfig, TsyncProviderConfig>;
TYPED_TEST_SUITE(SharedMemTimeBaseWriterConfigWriteTestFixture, WriteConfigTypes);

namespace testing {
namespace lib_sharedmemtimebasewriter_ut {

// Test whether Open will open shared memory in case of success case.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_HappyPath_Succeeds) {
    // Act and assert
    tb_writer_->Open();
    ASSERT_EQ(tb_writer_->GetState(), ITimeBaseAccessor::State::Open);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, Open_OnMaxSizeTooSmall_Aborts) {
    // Arrange
    size_t* max_size = const_cast<size_t*>(&tb_writer_->max_size_);
    *max_size = sizeof(TsyncReadWriteLock);

    ASSERT_EXIT(
        {
            // Act
            tb_writer_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "MaxSize is too small");
}

// Test whether Unlink executes correctly if shmem is owned.
TEST_F(SharedMemTimeBaseWriterTestFixture, Close_WhenOwning_CallsUnlink) {
    // Arrange
    tb_writer_->is_owner_ = true;
    tb_writer_->Open();

    EXPECT_CALL(*shared_mem_mock, OsShmUnlink(::testing::_)).Times(1);

    // Act
    tb_writer_->Close();
}

// Test whether Unlink executes correctly if shmem is owned.
TEST_F(SharedMemTimeBaseWriterTestFixture, Close_WhenOpenAndAddressIsNull_Succeeds) {
    // Arrange
    tb_writer_->is_owner_ = true;
    tb_writer_->Open();
    tb_writer_->addr_ = nullptr;

    EXPECT_CALL(*shared_mem_mock, OsMunmap(::testing::_, ::testing::_)).Times(0);

    // Act
    tb_writer_->Close();
}

// Test whether Unlink executes correctly if OsShmUnlink fails.
TEST_F(SharedMemTimeBaseWriterTestFixture, Unlink_OnFailure_Succeeds) {
    // Arrange
    std::shared_ptr<SharedMemTimeBaseWriter> p_tb_writer =
        std::make_shared<SharedMemTimeBaseWriter>("TestWriter/");
    p_tb_writer->is_owner_ = true;
    EXPECT_CALL(*shared_mem_mock, OsShmUnlink(::testing::_)).Times(1).WillOnce(Return(1));

    // Act
    tb_writer_->Unlink();
}

// Test whether Open will open shared memory in case of success case if the name has a trailing /.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_SuccessCase_OpenSharedMemWithTrailingSlash) {
    // Act and assert
    SetUpWriter(shared_mem_name_ + "/");
    EXPECT_NO_THROW(tb_writer_->Open());
}

// Test whether WriteDefaults will succeed
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteDefaults_Succeeds) {
    // Act
    tb_writer_->Open();
    auto res = tb_writer_->WriteDefaults();

    // Assert
    ASSERT_TRUE(res);
}

// Test whether WriteDefaults will succeed
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteDefaults_OnCloseddWriter_Fails) {
    // Act
    auto res = tb_writer_->WriteDefaults();

    // Assert
    ASSERT_FALSE(res);
}

// Test whether Open will open shared memory in case of success case.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_AlreadyOpened_WillNotOpenSharedMemory) {
    // Arrange

    tb_writer_->Open();
    ASSERT_TRUE(tb_writer_->GetState() == ITimeBaseAccessor::State::Open);

    EXPECT_CALL(*shared_mem_mock, OsShmOpen(::testing::_, ::testing::_, ::testing::_)).Times(0);

    // Act and assert
    tb_writer_->Open();
}

// Test whether Open will abort in case if OsShmOpen will fail and return -1.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_FailedToOpenSharedMemory_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock, OsShmOpen(::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(-1));

            // Act
            tb_writer_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Unable to open shared memory file");
}

// Test whether Open will abort in case if ftruncate fails and returns -1.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_FailedTruncate_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock, OsFtruncate(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

            // act
            // OsFtruncate is only called for the owner
            tb_writer_->is_owner_ = true;
            tb_writer_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Unable to set shared memory block size");
}

// Test whether Open will abort in case if mmap fails returns nullptr.
TEST_F(SharedMemTimeBaseWriterTestFixture, Open_MmapReturnsMapFailed_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock,
                        OsMmap(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(MAP_FAILED));

            // act
            tb_writer_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "OsMmap failed");
}

// Test whether lock will abort when called on a closed writer.
TEST_F(SharedMemTimeBaseWriterTestFixture, lock_OnClosedWriter_Aborts) {
    // Arrange
    ASSERT_EXIT({ tb_writer_->lock(); }, ::testing::ExitedWithCode(EXIT_CODE), "lock called on closed writer");
}

TEST_F(SharedMemTimeBaseWriterTestFixture, lock_OnMaxSizeTooSmall_Aborts) {
    // Arrange
    tb_writer_->Open();
    size_t* max_size = const_cast<size_t*>(&tb_writer_->max_size_);
    *max_size = 10u;
    ASSERT_EXIT({ tb_writer_->lock(); }, ::testing::ExitedWithCode(EXIT_CODE), "MaxSize is too small");
}

// Test whether unlock will abort when called on a closed writer.
TEST_F(SharedMemTimeBaseWriterTestFixture, unlock_OnClosedWriter_Aborts) {
    // Arrange
    ASSERT_EXIT({ tb_writer_->unlock(); }, ::testing::ExitedWithCode(EXIT_CODE), "unlock called on closed writer");
}

// Test whether Close will output error in case munmap fails.
TEST_F(SharedMemTimeBaseWriterTestFixture, Close_MunmapReturnsError_OutputsMessage) {
    // Arrange
    tb_writer_->Open();
    testing::internal::CaptureStderr();
    EXPECT_CALL(*shared_mem_mock, OsMunmap(::testing::_, ::testing::_)).WillOnce(::testing::Return(-1));

    // Act
    tb_writer_->Close();

    // Assert
    auto output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, ::testing::MatchesRegex(".*munmap failed with.*"));
}

// Test whether Close will succeed if Open was not called previously
TEST_F(SharedMemTimeBaseWriterTestFixture, Close_WithoutOpenCall_Succeeds) {
    // Arranged by fixture

    // Act
    tb_writer_->Close();

    // Assert
    SUCCEED();
}

// Test whether GetName will return correct name of shared memory name.
TEST_F(SharedMemTimeBaseWriterTestFixture, GetName_SuccessCase_ReturnCorrectName) {
    // Act
    auto name = tb_writer_->GetName();

    // Assert
    ASSERT_EQ(name, shared_mem_name_);
}

// Test whether GetAccessor will return a valid instance
TEST_F(SharedMemTimeBaseWriterTestFixture, GetAccessor_OnCall_ReturnsValidInstance) {
    // Act
    auto& acc = tb_writer_->GetAccessor();

    // Assert
    ASSERT_EQ(acc.GetName(), tb_writer_->GetName());
    ASSERT_EQ(acc.GetState(), tb_writer_->GetState());
}

// Test that writing of an empty byte span will succeed.
TEST_F(SharedMemTimeBaseWriterTestFixture, Write_EmptyByteSpan_Succeeds) {
    // Arrange
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    auto pos = tb_writer_->GetPosition();
    auto res = tb_writer_->Write(span<std::byte>());
    tb_writer_->unlock();

    // Assert
    ASSERT_TRUE(res);

    uint8_t* p = &mem_region_[pos];
    // compare size
    ASSERT_EQ(static_cast<uint32_t>(*p), 0);
}

// Test that writing a byte span to a closed writer will fail
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteSpan_ToClosedWriter_Fails) {
    // Act
    ASSERT_EQ(ITimeBaseAccessor::State::Closed, tb_writer_->GetState());
    auto res = tb_writer_->Write(span<std::byte>());

    // Assert
    ASSERT_FALSE(res);
}

// Test that writing an std::string to a closed writer will fail
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteString_ToClosedWriter_Fails) {
    // Act
    auto res = tb_writer_->Write(std::string());

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteString_OnLengthFieldOutOfBounds_Fails) {
    // Arrange
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 1u);

    // Act
    auto res = tb_writer_->Write(std::string{});

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteString_OnStringDataOutOfBounds_Fails) {
    // Arrange
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 12u);
    std::string test{"testtesttestestest"};

    // Act
    auto res = tb_writer_->Write(test);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteString_OnTerminatorOutOfBounds_Fails) {
    // Arrange
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 20u);
    Align<ITimeBaseWriter, std::uint64_t>(*tb_writer_.get());
    std::size_t space_left = tb_writer_->GetMaxSize() - tb_writer_->GetPosition() - sizeof(std::uint64_t);

    ASSERT_NE(space_left, 0u);

    std::string test{"testtesttesttesttesttesttest"};

    // Act
    auto res = tb_writer_->Write(test.substr(space_left));

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteStringView_ToClosedWriter_Fails) {
    // Act
    auto res = tb_writer_->Write(std::string_view());

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteStringView_ToOpenWriter_Succeeds) {
    // Act
    tb_writer_->Open();
    auto res = tb_writer_->Write(std::string_view("Test"));
    // Assert
    ASSERT_TRUE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteStringView_OnSizeFieldOutOfBounds_Fails) {
    // Act
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 2u);
    auto res = tb_writer_->Write(std::string_view("Test"));
    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteStringView_OnDataOutOfBounds_Fails) {
    // Act
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 20u);
    auto res = tb_writer_->Write(std::string_view("TestTestTestTestTestTestTestTest"));
    // Assert
    ASSERT_FALSE(res);
}

// Test that writing the consumer config to a closed writer fails.
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteConsumerConfig_ToClosedWriter_Fails) {
    // arrange
    TsyncConsumerConfig cfg;

    // act
    auto res = tb_writer_->Write(cfg);

    // assert
    ASSERT_FALSE(res);
}

// Test that writing to a closed writer fails.
TEST_F(SharedMemTimeBaseWriterTestFixture, WriteProviderConfig_ToClosedWriter_Fails) {
    // arrange
    TsyncProviderConfig cfg;

    // act
    auto res = tb_writer_->Write(cfg);

    // assert
    ASSERT_FALSE(res);
}

// Test that writing of an non-empty byte span will succeed.
TEST_F(SharedMemTimeBaseWriterTestFixture,
       Write_ByteSpan_Succeeds) {  // Arrange
    constexpr auto span_data = make_array<std::byte>(1, 2, 3);
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    auto pos = tb_writer_->GetPosition();
    auto res = tb_writer_->Write(span<const std::byte>(span_data));
    tb_writer_->unlock();

    // Assert
    ASSERT_TRUE(res);

    // compare size
    ASSERT_EQ(static_cast<std::size_t>(mem_region_[pos]), sizeof(span_data));
    // compare bytes
    for (auto b : span_data) {
        ASSERT_EQ(static_cast<uint32_t>(mem_region_[++pos]), static_cast<uint32_t>(static_cast<uint8_t>(b)));
    }
}  // namespace lib_sharedmemtimebasewriter_ut

// Test that SetPosition will fail when called on closed writer.
TEST_F(SharedMemTimeBaseWriterTestFixture, SetPosition_MemoryNotOpened_Fails) {
    // Act
    auto res = tb_writer_->SetPosition(10);

    // ASSERT
    ASSERT_FALSE(res);
}

// Test whether GetState will return correct state of shared memory.
TEST_F(SharedMemTimeBaseWriterTestFixture, GetState_MemoryNotOpened_ReturnCorrectState) {
    // Act
    auto res = tb_writer_->GetState();

    // Assert
    ASSERT_EQ(res, ITimeBaseAccessor::State::Closed);
}

// Test whether GetState will return correct state of shared memory after open call.
TEST_F(SharedMemTimeBaseWriterTestFixture, GetState_MemoryOpened_ReturnCorrectState) {
    // Arrange
    tb_writer_->Open();

    // Act
    auto res = tb_writer_->GetState();

    // Assert
    ASSERT_EQ(res, ITimeBaseAccessor::State::Open);
}

// Test whether Write writes correct time stamp data and returns true.
TEST_F(SharedMemTimeBaseWriterTestFixture, Write_ValidTimeStampData_ReturnTrue) {
    // Arrange
    auto reader = std::make_unique<SharedMemTimeBaseReader>(this->shared_mem_name_);
    TimestampWithStatus data = {SynchronizationStatus::kSynchronized, std::chrono::nanoseconds(12),
                                std::chrono::seconds(34)};
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    auto res = tb_writer_->Write(data);
    tb_writer_->unlock();

    // Assert
    ASSERT_EQ(res, true);

    TimestampWithStatus read_data;
    reader->Open();
    reader->lock();
    reader->Read(read_data);
    reader->unlock();

    ASSERT_EQ(read_data.status, data.status);
    ASSERT_EQ(read_data.nanoseconds, data.nanoseconds);
    ASSERT_EQ(read_data.seconds, data.seconds);
}

// Test whether Write writes correct virtual local time and returns true.
TEST_F(SharedMemTimeBaseWriterTestFixture, Write_WithValidVirtualLocaltime_Succeeds) {
    // Arrange
    auto reader = std::make_unique<SharedMemTimeBaseReader>(this->shared_mem_name_);
    VirtualLocalTime data(1234);
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    auto res = tb_writer_->Write(data);
    tb_writer_->unlock();

    // Assert
    ASSERT_EQ(res, true);

    VirtualLocalTime read_data;
    reader->Open();
    reader->lock();
    reader->Read(read_data);
    reader->unlock();

    ASSERT_EQ(data, read_data);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, Write_WithZeroLengthRawData_Succeeds) {
    // Arrange
    auto reader = std::make_unique<SharedMemTimeBaseReader>(this->shared_mem_name_);
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    auto res = tb_writer_->Write(nullptr, 0u);
    tb_writer_->unlock();

    // Assert
    ASSERT_TRUE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, Write_WithOutOfBoudsRawData_Succeeds) {
    // Arrange
    auto reader = std::make_unique<SharedMemTimeBaseReader>(this->shared_mem_name_);
    tb_writer_->Open();

    // Act
    tb_writer_->lock();
    tb_writer_->SetPosition(kSharedMemPageSize - 2u);
    auto res = tb_writer_->Write(nullptr, 4u);
    tb_writer_->unlock();

    // Assert
    ASSERT_FALSE(res);
}

// Test whether Skip returns false in case memory not opened yet.
TEST_F(SharedMemTimeBaseWriterTestFixture, Skip_OnClosedWriter_ReturnFalse) {
    // Act
    auto res = tb_writer_->Skip(10);

    // Assert
    ASSERT_EQ(res, false);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, Skip_OnOutOfBounds_ReturnFalse) {
    // Arrange
    tb_writer_->Open();
    tb_writer_->SetPosition(tb_writer_->GetMaxSize() - 4u);

    // Act
    auto res = tb_writer_->Skip(10);

    // Assert
    ASSERT_EQ(res, false);
}

// Test whether Skip returns true and increments offset if the memory was opened.
TEST_F(SharedMemTimeBaseWriterTestFixture, Skip_OnOpenWriter_IncrementOffset) {
    // Arrange
    tb_writer_->Open();
    std::size_t initial_offset = sizeof(score::time::TsyncReadWriteLock);
    std::size_t skip_bytes_count = 10;
    uint8_t test = 42;

    // Act
    tb_writer_->lock();
    auto res = tb_writer_->Skip(skip_bytes_count);
    ASSERT_EQ(initial_offset + skip_bytes_count, tb_writer_->GetPosition());
    tb_writer_->Write(test);
    tb_writer_->unlock();

    // Assert
    ASSERT_EQ(res, true);
    ASSERT_EQ(mem_region_[initial_offset + skip_bytes_count], test);
}

// Test whether Read is capable to read plain data types.
TYPED_TEST(SharedMemTimeBaseWriterWriteTestFixture, Write_Succeeds) {
    // Arrange
    this->tb_writer_->Open();

    // Act
    this->tb_writer_->lock();
    auto res = this->tb_writer_->Write(this->value_);
    this->tb_writer_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(this->value_, *(this->mem_));
}

TYPED_TEST(SharedMemTimeBaseWriterWriteTestFixture, Write_OnClosedWriter_Fails) {
    // Arrange
    this->tb_writer_->Close();

    // Act
    auto res = this->tb_writer_->Write(this->value_);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteUserData_OnClosedWriter_Fails) {
    // Arrange
    auto p_tb_writer = std::make_shared<SharedMemTimeBaseWriter>(this->shared_mem_name_);
    UserData data;
    data[0] = std::byte{0};

    auto res = p_tb_writer->Write(data);

    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteUserData_OnWriteFieldFailure_Fails) {
    // Arrange
    auto tb_writer = std::make_shared<SharedMemTimeBaseWriter>(this->shared_mem_name_);
    tb_writer->SetPosition(kSharedMemPageSize - 2);

    UserData data;
    data[0] = std::byte{3};
    data[1] = std::byte{0};
    data[2] = std::byte{0};
    data[3] = std::byte{0};

    auto res = tb_writer->Write(data);

    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteUserData_OnIllegalSize_Succeeds) {
    // Arrange
    auto tb_writer = std::make_shared<SharedMemTimeBaseWriter>(this->shared_mem_name_);

    constexpr auto data = make_array<std::byte>(1, 2, 3, 4, 5);
    tb_writer->Open();
    tb_writer->lock();

    // act
    auto res = tb_writer->Write(UserDataView{data});

    // assert
    ASSERT_TRUE(res);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteUserData_OnOutOfBounds_Fails) {
    // Arrange
    auto tb_writer = std::make_shared<SharedMemTimeBaseWriter>(this->shared_mem_name_);

    constexpr auto data = make_array<std::byte>(1, 2, 3, 4, 5);
    tb_writer->Open();
    tb_writer->lock();
    tb_writer->SetPosition(kSharedMemPageSize - 2u);

    // act
    auto res = tb_writer->Write(UserDataView{data});

    // assert
    ASSERT_FALSE(res);
}

// Test whether Read for complex data types returns false if memory was not opened before.
TYPED_TEST(SharedMemTimeBaseWriterComplexWriteTestFixture, Write_Succeeds) {
    // Arrange
    auto reader = std::make_unique<SharedMemTimeBaseReader>(this->shared_mem_name_);
    this->tb_writer_->Open();
    this->struct_ = TypeParam{};

    // Act
    this->tb_writer_->lock();
    auto res = this->tb_writer_->Write(this->struct_);
    ASSERT_TRUE(res);
    this->tb_writer_->unlock();

    decltype(this->struct_) read_data{};
    reader->Open();
    reader->lock();
    res = reader->Read(read_data);
    ASSERT_TRUE(res);
    reader->unlock();

    // Assert
    ASSERT_EQ(memcmp(&read_data, &this->struct_, sizeof(this->struct_)), 0);
}

void fillConfig(TsyncConsumerConfig& cfg) {
    cfg.name = "test";
    cfg.time_slave_config.follow_up_timeout_value = std::chrono::milliseconds(0xf1);
    cfg.time_slave_config.time_leap_future_threshold = std::chrono::milliseconds(0xf2);
    cfg.time_slave_config.time_leap_past_threshold = std::chrono::milliseconds(0xf3);
    cfg.time_slave_config.time_leap_healing_counter = 0xf4;
    cfg.time_slave_config.global_time_sequence_counter_jump_width = 0xf5;
    cfg.time_slave_config.sub_tlv_config.time_enabled = true;
    cfg.time_slave_config.sub_tlv_config.status_enabled = false;
    cfg.time_slave_config.sub_tlv_config.user_data_enabled = true;
    cfg.time_slave_config.is_valid = true;
}

void fillConfig(TsyncProviderConfig& cfg) {
    cfg.name = "test";
    cfg.time_master_config.immediate_resume_time = std::chrono::milliseconds(42);
    cfg.time_master_config.is_system_wide_global_time_master = true;
    cfg.time_master_config.is_valid = true;
    cfg.time_master_config.sync_period = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.num_rate_corrections_per_measurement_duration = 42U;
    cfg.time_sync_correction_config.offset_correction_adaption_interval = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.offset_correction_jump_threshold = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.rate_deviation_measurement_duration = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.allow_provider_rate_correction = true;
    cfg.time_sync_correction_config.is_valid = true;
}

void fillConfig(TsyncEthTimeDomain& cfg) {
    cfg.num_fup_data_id_entries = 16;
    cfg.vlan_priority = 2;
    cfg.message_format = TsyncEthMessageFormat::kIeee8021ASAutosar;
    cfg.is_valid = true;
}

void fillConfig(TsyncTimeDomainConfig& cfg) {
    cfg.domain_id = 12;
    cfg.debounce_time = std::chrono::milliseconds(1000);
    cfg.sync_loss_timeout = std::chrono::milliseconds(1000);
    fillConfig(cfg.eth_time_domain);
    fillConfig(cfg.consumer_config);
    fillConfig(cfg.provider_config);
}

TYPED_TEST(SharedMemTimeBaseWriterConfigWriteTestFixture, Write_Succeeds) {
    // Arrange
    fillConfig(this->written_struct_);

    // Act
    this->tb_writer_->Open();
    this->tb_writer_->lock();
    auto res_write = this->tb_writer_->Write(this->written_struct_);
    this->tb_writer_->unlock();

    SharedMemTimeBaseReader reader(this->tb_writer_->GetName());
    reader.Open();
    reader.lock();
    auto res_read = reader.Read(this->read_struct_);
    reader.unlock();

    // Assert
    ASSERT_TRUE(res_write);
    ASSERT_TRUE(res_read);
    ASSERT_EQ(this->read_struct_, this->written_struct_);
}

TYPED_TEST(SharedMemTimeBaseWriterConfigWriteTestFixture, Write_OnPositionEof_Fails) {
    // Arrange
    fillConfig(this->written_struct_);

    // Act
    this->tb_writer_->Open();
    this->tb_writer_->lock();
    std::size_t& max_size_ref = const_cast<std::size_t&>(this->tb_writer_->max_size_);
    max_size_ref = 0;
    auto res_write = this->tb_writer_->Write(this->written_struct_);
    this->tb_writer_->unlock();

    // Assert
    ASSERT_FALSE(res_write);
}

TYPED_TEST(SharedMemTimeBaseWriterConfigWriteTestFixture, Write_OnClosed_Fails) {
    // Arrange
    fillConfig(this->written_struct_);
    this->tb_writer_->Close();

    // Act
    auto res_write = this->tb_writer_->Write(this->written_struct_);

    // Assert
    ASSERT_FALSE(res_write);
}

TEST(Lib_SharedMemTimeBaseWriter, Construction_With_MissingLeadingSlashInName_CorrectsName) {
    // Arrange
    std::string writer_name = "TestWriter";
    std::string corrected_writer_name = "/" + writer_name;

    // Act
    SharedMemTimeBaseWriter writer(writer_name);

    // Assert
    ASSERT_EQ(writer.shm_name_, corrected_writer_name);
}

TEST_F(SharedMemTimeBaseWriterTestFixture, WriteField_OnOutOfBounds_Fails) {
    // Arrange
    std::size_t max_size = 100u;
    auto tb_writer = std::make_shared<SharedMemTimeBaseWriter>(this->shared_mem_name_, max_size);
    uint32_t data{10};
    tb_writer->Open();
    tb_writer->SetPosition(max_size - 1u);
    auto res = tb_writer->WriteField(data);
    ASSERT_EQ(res, 0);
}

}  // namespace lib_sharedmemtimebasewriter_ut
}  // namespace testing
