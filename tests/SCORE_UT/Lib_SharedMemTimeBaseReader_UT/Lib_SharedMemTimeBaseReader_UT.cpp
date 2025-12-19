/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <gtest/gtest.h>
#include <sys/mman.h>

#include <cstring>
#include <mutex>

//!!#include "ara/core/abort.h"
#include "score/span.hpp"
#define private public
#include "score/time/utility/TsyncConfigTypes.h"
#include "score/time/utility/SharedMemTimeBaseReader.h"
#include "score/time/utility/SharedMemTimeBaseWriter.h"
#undef private
#include "matcher_operators.h"
#include "SysCallsReadWriteLockMock.h"
#include "SysCallsShMemMock.h"

using score::cpp::span;
using namespace score::time;

std::unique_ptr<::testing::NiceMock<SysCallsShMemMock>> score::time::shared_mem_mock;
std::unique_ptr<testing::NiceMock<SysCallsReadWriteLockMock>> score::time::rw_lock_mock;

class SharedMemTimeBaseReaderTestFixture : public ::testing::Test {
   public:
    static const int32_t EXIT_CODE;

   protected:
    void SetUp() override {
        tb_reader_ = std::make_unique<SharedMemTimeBaseReader>(shared_mem_name_);
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
        tb_reader_.reset();
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
    char mem_region_[4096];
    std::unique_ptr<SharedMemTimeBaseReader> tb_reader_;
};

template <class Type>
class SharedMemTimeBaseReaderReadComplexTypeTestFixture : public SharedMemTimeBaseReaderTestFixture {
   protected:
    Type struct_;
    Type read_data_;
};

template <class Type>
class SharedMemTimeBaseReaderReadConfigTypeTestFixture : public SharedMemTimeBaseReaderTestFixture {
   protected:
    Type struct_;
    Type read_data_;
};

template <class Type>
class SharedMemTimeBaseReaderReadTestFixture : public SharedMemTimeBaseReaderTestFixture {
   protected:
    Type value_ = 42;
    Type read_value_ = 0;
};

constexpr const int32_t SharedMemTimeBaseReaderTestFixture::EXIT_CODE = 42;

using ReadComplexTypes = ::testing::Types<TimestampWithStatus, SynchronizationStatus>;
TYPED_TEST_SUITE(SharedMemTimeBaseReaderReadComplexTypeTestFixture, ReadComplexTypes);

using ReadConfigTypes = ::testing::Types<TsyncTimeDomainConfig, TsyncConsumerConfig, TsyncProviderConfig>;
TYPED_TEST_SUITE(SharedMemTimeBaseReaderReadConfigTypeTestFixture, ReadConfigTypes);

using ReadTypes =
    ::testing::Types<float, double, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;
TYPED_TEST_SUITE(SharedMemTimeBaseReaderReadTestFixture, ReadTypes);

namespace testing {
namespace lib_sharedmemtimebasereader_ut {

// Test whether Open will open shared memory in case of success case.
TEST_F(SharedMemTimeBaseReaderTestFixture, Open_Succeeds) {
    // Act and assert
    EXPECT_NO_THROW(tb_reader_->Open());
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Open_OnMaxSizeTooSmall_Aborts) {
    // Arrange
    std::size_t *sz = const_cast<std::size_t *>(&tb_reader_->max_size_);
    *sz = sizeof(TsyncReadWriteLock);

    // Act and assert
    ASSERT_EXIT(
        {
            // Act
            tb_reader_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "MaxSize is too small");
}

// Test whether Open will open shared memory in case of success case.
TEST_F(SharedMemTimeBaseReaderTestFixture, Open_AlreadyOpened_WillNotOpenSharedMemory) {
    // Arrange
    tb_reader_->Open();

    EXPECT_CALL(*shared_mem_mock, OsShmOpen(::testing::_, ::testing::_, ::testing::_)).Times(0);

    // Act and assert
    tb_reader_->Open();
}

// Test whether Open will abort in case if shm_open will fail and returns -1.
TEST_F(SharedMemTimeBaseReaderTestFixture, Open_FailedToOpenSharedMemory_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock, OsShmOpen(::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(-1));

            // Act
            tb_reader_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Unable to open shared memory file");
}

// Test whether lock will abort if called on a closed reader
TEST_F(SharedMemTimeBaseReaderTestFixture, lock_OnClosedReader_Aborts) {
    ASSERT_EXIT({ tb_reader_->lock(); }, ::testing::ExitedWithCode(EXIT_CODE), "lock called on closed reader");
}

TEST_F(SharedMemTimeBaseReaderTestFixture, lock_OnMaxSizeTooSmall_Aborts) {
    tb_reader_->Open();
    std::size_t *max_size = const_cast<std::size_t *>(&tb_reader_->max_size_);
    *max_size = sizeof(TsyncReadWriteLock) - 4u;
    ASSERT_EXIT({ tb_reader_->lock(); }, ::testing::ExitedWithCode(EXIT_CODE), "MaxSize is too small");
}

// Test whether lock will abort if called on a closed reader
TEST_F(SharedMemTimeBaseReaderTestFixture, unlock_OnClosedReader_Aborts) {
    ASSERT_EXIT({ tb_reader_->unlock(); }, ::testing::ExitedWithCode(EXIT_CODE), "unlock called on closed reader");
}

TEST_F(SharedMemTimeBaseReaderTestFixture, AlignedSkip_BeyondBounds_Fails) {
    // Act and assert
    EXPECT_NO_THROW(tb_reader_->Open());

    ASSERT_TRUE(tb_reader_->SetPosition(4094));
    bool res = AlignedSkip<ITimeBaseReader, uint64_t>(*tb_reader_.get());
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Align_OnAlreadyAligned_KeepsPosition) {
    tb_reader_->Open();
    Align<ITimeBaseReader, uint32_t>(*tb_reader_.get());
    std::size_t pos1 = tb_reader_->GetPosition();
    Align<ITimeBaseReader, uint32_t>(*tb_reader_.get());
    std::size_t pos2 = tb_reader_->GetPosition();
    ASSERT_EQ(pos1, pos2);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Align_OnUnaligned_AlignsCorrectly) {
    tb_reader_->Open();
    ASSERT_TRUE(tb_reader_->Skip(3));
    std::size_t pos = tb_reader_->GetPosition();
    Align<ITimeBaseReader, uint32_t>(*tb_reader_.get());
    std::size_t pos_aligned = tb_reader_->GetPosition();
    ASSERT_NE(pos, pos_aligned);
    ASSERT_EQ(0u, pos_aligned % alignof(uint32_t));
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Align_OnOutOfBounds_Fails) {
    // Act and assert
    struct TestStruct {
        char a;
        char b;
        char c;
        uint64_t d;
    };
    tb_reader_->Open();
    ASSERT_TRUE(tb_reader_->SetPosition(tb_reader_->GetMaxSize() - 2u));
    auto pos_before = tb_reader_->GetPosition();
    ASSERT_FALSE((Align<ITimeBaseReader, TestStruct>(*tb_reader_)));
    auto pos_after = tb_reader_->GetPosition();
    ASSERT_EQ(pos_before, pos_after);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, AlignedSkip_WhenUnaligned_Succeeds) {
    // Act and assert
    EXPECT_NO_THROW(tb_reader_->Open());
    ASSERT_TRUE(tb_reader_->Skip(1));
    bool res = AlignedSkip<ITimeBaseReader, uint64_t>(*tb_reader_.get());
    ASSERT_TRUE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, AlignedSkip_Succeeds) {
    // Act and assert
    tb_reader_->Open();

    bool res = AlignedSkip<ITimeBaseReader, uint64_t>(*tb_reader_.get());
    ASSERT_TRUE(res);
}

// Test whether Open will abort in case if the first call to mmap fails and returns nullptr.
TEST_F(SharedMemTimeBaseReaderTestFixture, Open_MmapProtWriteReturnsMapFailed_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock,
                        OsMmap(::testing::_, ::testing::_, PROT_WRITE, ::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(MAP_FAILED));

            // Act
            tb_reader_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "OsMmap failed");
}

// Test whether Open will abort in case if the second call to mmap fails and returns nullptr.
TEST_F(SharedMemTimeBaseReaderTestFixture, Open_MmapProtReadReturnsMapFailed_Aborts) {
    // Arrange
    ASSERT_EXIT(
        {
            EXPECT_CALL(*shared_mem_mock,
                        OsMmap(::testing::_, ::testing::_, PROT_READ, ::testing::_, ::testing::_, ::testing::_))
                .WillOnce(::testing::Return(MAP_FAILED));

            // Act
            tb_reader_->Open();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "OsMmap failed");
}

// Test whether Close will output message to stderr if munmap fails
TEST_F(SharedMemTimeBaseReaderTestFixture, Close_MunmapFails_OutputsError) {
    // Arrange
    tb_reader_->Open();
    testing::internal::CaptureStderr();
    EXPECT_CALL(*shared_mem_mock, OsMunmap(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(-1));

    // Act
    tb_reader_->Close();

    // Assert
    auto output = testing::internal::GetCapturedStderr();
    EXPECT_THAT(output, ::testing::MatchesRegex(".*munmap failed with.*"));
}

// Test that when calling Close with an invalid internal state, no errors happen
TEST_F(SharedMemTimeBaseReaderTestFixture, Close_WithInvalidState_Succeeds) {
    // Arrange
    tb_reader_->state_ = ITimeBaseAccessor::State::Open;

    // Act
    tb_reader_->Close();

    // ASSERT
    SUCCEED();
}

// Test whether GetAccessor will return a valid instance
TEST_F(SharedMemTimeBaseReaderTestFixture, GetAccessor_OnCall_ReturnsValidInstance) {
    // Act
    auto &acc = tb_reader_->GetAccessor();

    // Assert
    ASSERT_EQ(acc.GetName(), tb_reader_->GetName());
    ASSERT_EQ(acc.GetState(), tb_reader_->GetState());
}

// Test that SetPosition will fail when called on closed writer.
TEST_F(SharedMemTimeBaseReaderTestFixture, SetPosition_MemoryNotOpened_Fails) {
    // Act
    auto res = tb_reader_->SetPosition(10);

    // ASSERT
    ASSERT_FALSE(res);
}

// Test whether GetName will return correct name of shared memory name.
TEST_F(SharedMemTimeBaseReaderTestFixture, GetName_SucessCase_ReturnCorrectName) {
    // Act
    auto name = tb_reader_->GetName();

    // Assert
    ASSERT_EQ(name, shared_mem_name_);
}

// Test whether GetState will return correct state of shared memory.
TEST_F(SharedMemTimeBaseReaderTestFixture, GetState_MemoryNotOpened_ReturnCorrectState) {
    // Act
    auto res = tb_reader_->GetState();

    // Assert
    ASSERT_EQ(res, ITimeBaseAccessor::State::Closed);
}

// Test whether GetState will return correct state of shared memory after open call.
TEST_F(SharedMemTimeBaseReaderTestFixture, GetState_MemoryOpened_ReturnCorrectState) {
    // Arrange
    tb_reader_->Open();

    // Act
    auto res = tb_reader_->GetState();

    // Assert
    ASSERT_EQ(res, ITimeBaseAccessor::State::Open);
}

// Test whether Skip returns false in case memory not opened yet.
TEST_F(SharedMemTimeBaseReaderTestFixture, Skip_MemoryNotOpened_ReturnFalse) {
    // Act
    auto res = tb_reader_->Skip(10);

    // Assert
    ASSERT_EQ(res, false);
}

// Test whether Skip returns true and increments offset if the memory was opened.
TEST_F(SharedMemTimeBaseReaderTestFixture, Skip_MemoryOpened_IncrementOffset) {
    // Arrange
    tb_reader_->Open();
    std::size_t initial_offset = sizeof(TsyncReadWriteLock);
    std::size_t skip_bytes_count = 10;
    uint8_t test = 42;
    uint8_t read_data = 0;
    mem_region_[initial_offset + skip_bytes_count] = test;

    // Act
    auto res = tb_reader_->Skip(skip_bytes_count);
    tb_reader_->Read(read_data);

    // Assert
    ASSERT_EQ(res, true);
    ASSERT_EQ(read_data, test);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Skip_OnOutOfBounds_ReturnsFalse) {
    // Arrange
    tb_reader_->Open();

    // Act
    bool result = tb_reader_->Skip(1000000u);

    // Assert
    ASSERT_FALSE(result);
}
// Test that reading to a void pointer fails with a closed reader
TEST_F(SharedMemTimeBaseReaderTestFixture, ReadVoidPointer_WithClosedReader_Fails) {
    // Arrange
    const char *p;

    // Act
    auto res = tb_reader_->Read(&p, 1);

    // ASSERT
    ASSERT_FALSE(res);
}

// Test that an empty span can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_EmptyUserData_Succeeds) {
    // Arrange
    tb_reader_->Open();

    // set size of span
    mem_region_[tb_reader_->GetPosition()] = 0;

    // Act
    tb_reader_->lock();
    UserDataView v;
    auto res = tb_reader_->Read(v);
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(v.size(), 0);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Read_UserDataWithInvalidSize_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    writer->Write(static_cast<uint8_t>(5u));

    tb_reader_->Open();

    // Act
    UserDataView v;
    auto res = tb_reader_->Read(v);

    // Assert
    ASSERT_FALSE(res);
}

// Test that reading a string on a closed reader fails
TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_WithClosedReader_Fails) {
    // Arrange & Act
    std::string str{};
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_FALSE(res);
    ASSERT_TRUE(str.empty());
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_OnStringLengthFieldOutOfounds_Fails) {
    // Arrange
    tb_reader_->Open();
    std::size_t offset = tb_reader_->GetMaxSize() - 7u;
    tb_reader_->SetPosition(offset);

    // Act
    std::string str{};
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_OnStringDataOutOfounds_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    std::size_t offset = writer->GetMaxSize() - 18u;
    constexpr uint64_t str_len = 11;
    writer->SetPosition(offset);
    ASSERT_TRUE(writer->Write(str_len));

    tb_reader_->Open();
    tb_reader_->SetPosition(offset);

    // Act
    std::string str{};
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_OnStringLengthOutOfounds_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    uint64_t str_len = writer->GetMaxSize();
    std::size_t offset = writer->GetMaxSize() - str_len - sizeof(str_len);
    writer->SetPosition(offset);
    ASSERT_TRUE(writer->Write(str_len));

    tb_reader_->Open();
    tb_reader_->SetPosition(offset);

    // Act
    std::string str{};
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_FALSE(res);
}

// Test that an empty std::string can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_EmptyString_Succeeds) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    ASSERT_TRUE(writer->Write(std::string()));

    tb_reader_->Open();

    // Act
    std::string str{};
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_TRUE(res);
    ASSERT_TRUE(str.empty());
}

// Test that an std::string can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_Succeeds) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    const std::string s("TEST");

    writer->Open();
    writer->lock();
    writer->Write(s);
    writer->unlock();

    tb_reader_->Open();

    // Act
    tb_reader_->lock();
    std::string str;
    auto res = tb_reader_->Read(str);
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(str, s);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_WithInvalidLength_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    const std::string s("TEST");

    writer->Open();
    writer->lock();
    std::size_t pos = writer->GetPosition();
    writer->Write(s);
    writer->unlock();

    // cut off string early
    mem_region_[pos + sizeof(uint64_t) + 3] = 0;

    tb_reader_->Open();

    // Act
    tb_reader_->lock();
    std::string str;
    auto res = tb_reader_->Read(str);
    tb_reader_->unlock();

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_OnOutOfBounds_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    std::size_t offset = writer->GetMaxSize() - sizeof(uint64_t);
    constexpr std::size_t size = 10u;

    writer->Open();
    writer->SetPosition(offset);
    writer->Write(size);

    tb_reader_->Open();
    tb_reader_->SetPosition(offset);

    // Act
    std::string str;
    auto res = tb_reader_->Read(str);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadString_OnOutOfBounds2_Fails) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    std::size_t offset = writer->GetMaxSize() - sizeof(uint64_t);
    constexpr std::size_t size = 5u;
    writer->Open();
    // write string length at the very end of the shared memory region
    writer->SetPosition(offset);
    writer->Write(size);
    tb_reader_->Open();

    // Act
    std::string s;
    // set read offset so the string length can be read successfully
    tb_reader_->SetPosition(offset);
    auto res = tb_reader_->Read(s);

    // Assert
    ASSERT_FALSE(res);
}

// Test that an empty std::string_view can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_Emptystd::string_view_Succeeds) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    writer->lock();
    writer->Write(std::string_view());
    writer->unlock();
    tb_reader_->Open();

    // Act
    tb_reader_->lock();
    std::string_view string_view;
    auto res = tb_reader_->Read(string_view);
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_TRUE(string_view.empty());
}

// Test that an std::string_view can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_std::string_view_Succeeds) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    const std::string_view sv("TEST");
    writer->Open();
    writer->lock();
    writer->Write(sv);
    writer->unlock();
    tb_reader_->Open();

    // Act
    tb_reader_->lock();
    std::string_view read_sv;
    auto res = tb_reader_->Read(read_sv);
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(sv, read_sv);
}

// Test that an std::string_view cannot be read on a closed reader
TEST_F(SharedMemTimeBaseReaderTestFixture, Readstd::string_view_WithClosedReader_Fails) {
    // Arrange

    // Act
    std::string_view read_sv;
    auto res = tb_reader_->Read(read_sv);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Readstd::string_view_OnLengthFieldOutOfBounds_Fails) {
    // Arrange
    tb_reader_->Open();
    tb_reader_->SetPosition(tb_reader_->GetMaxSize() - 1);

    // Act
    std::string_view read_sv;
    auto res = tb_reader_->Read(read_sv);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, Readstd::string_view_OnDataOutOfBounds_Fails) {
    // Arrange
    std::size_t offset = 10u;

    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();
    writer->lock();
    ASSERT_TRUE(writer->SetPosition(writer->GetMaxSize() - offset));
    // write stringview length at the very end of the shared memory region
    ASSERT_TRUE(writer->Write(static_cast<std::uint64_t>(8u)));
    writer->unlock();
    tb_reader_->Open();
    tb_reader_->lock();
    // set read offset so the stringview length can be read successfully
    ASSERT_TRUE(tb_reader_->SetPosition(tb_reader_->GetMaxSize() - offset));
    ASSERT_EQ(tb_reader_->GetMaxSize() - offset, tb_reader_->GetPosition());

    std::uint64_t read_size;
    ASSERT_TRUE(tb_reader_->Read(read_size));
    ASSERT_EQ(read_size, 8u);
    ASSERT_TRUE(tb_reader_->SetPosition(tb_reader_->GetMaxSize() - offset));
    // Act
    std::string_view read_sv;
    auto res = tb_reader_->Read(read_sv);
    tb_reader_->unlock();

    // Assert
    ASSERT_FALSE(res);
}

// Test that reading a span fails with a closed reader
TEST_F(SharedMemTimeBaseReaderTestFixture, ReadSpan_WithClosedReader_Fails) {
    // Arrange
    span<const std::byte> span;

    // Act
    auto res = tb_reader_->Read(span);

    // Assert
    ASSERT_FALSE(res);
}

// Test that ReadField on a closed reader fails
TEST_F(SharedMemTimeBaseReaderTestFixture, ReadField_WithClosedReader_Fails) {
    // Arrange
    int32_t dat;

    // Act
    auto res = tb_reader_->ReadField(dat);

    // Assert
    ASSERT_EQ(res, 0);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadProviderConfig_OnOutOfBounds_Fails) {
    // Arrange
    tb_reader_->Open();
    TsyncProviderConfig cfg;

    std::size_t *max_size = const_cast<std::size_t *>(&tb_reader_->max_size_);
    *max_size = kSharedMemTimebaseProviderConfigOffset - 2;

    // Act
    auto res = tb_reader_->Read(cfg);

    // Assert
    ASSERT_EQ(res, 0);
}

// Test that a span can be read successfully
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_Span_Succeeds) {
    // Arrange
    tb_reader_->Open();
    std::size_t pos = tb_reader_->GetPosition();
    static constexpr uint8_t span_size = 3;

    // set size of span
    mem_region_[pos] = span_size;
    // set span data
    mem_region_[pos + 1] = 1;
    mem_region_[pos + 2] = 2;
    mem_region_[pos + 3] = 3;

    // Act
    tb_reader_->lock();
    span<const std::byte> span;
    auto res = tb_reader_->Read(span);
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(span.size(), span_size);
    char *p = &mem_region_[pos + 1u];

    for (auto b : span) {
        ASSERT_EQ(static_cast<uint32_t>(*p), static_cast<uint32_t>(static_cast<uint8_t>(b)));
        ++p;
    }
}

// Test that a zero copy in place read succeeds
TEST_F(SharedMemTimeBaseReaderTestFixture, Read_ZeroCopy_Succeeds) {
    // Arrange
    tb_reader_->Open();
    std::size_t pos = tb_reader_->GetPosition();

    uint8_t data[]{1, 9, 4, 7, 43};

    std::memcpy(&mem_region_[pos], data, sizeof(data));

    // Act
    tb_reader_->lock();
    const char *p;
    auto res = tb_reader_->Read(&p, sizeof(data));
    tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(p, &mem_region_[pos]);

    for (auto b : data) {
        ASSERT_EQ(b, *p);
        ++p;
    }
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadDomainConfig_OnOutOfBounds_Fails) {
    // Arrange
    tb_reader_->Open();
    auto pos = tb_reader_->GetPosition();

    std::size_t *max_size = const_cast<std::size_t *>(&tb_reader_->max_size_);
    *max_size = pos + 10u;

    TsyncTimeDomainConfig cfg;

    // Act
    auto res = tb_reader_->Read(cfg);

    // Assert
    ASSERT_FALSE(res);
}

TEST_F(SharedMemTimeBaseReaderTestFixture, ReadConsumerConfig_OnOutOfBounds_Fails) {
    // Arrange
    tb_reader_->Open();
    auto pos = tb_reader_->GetPosition();

    std::size_t *max_size = const_cast<std::size_t *>(&tb_reader_->max_size_);
    *max_size = pos + 10u;

    TsyncConsumerConfig cfg;

    // Act
    auto res = tb_reader_->Read(cfg);

    // Assert
    ASSERT_FALSE(res);
}

// Test whether Read is capable to read plain data types.
TYPED_TEST(SharedMemTimeBaseReaderReadTestFixture, Read_CorrectData_ReturnCorrectData) {
    // Arrange
    this->tb_reader_->Open();
    std::size_t offset = this->tb_reader_->GetPosition();
    memcpy(&this->mem_region_[offset], &this->value_, sizeof(this->value_));

    // Act
    this->tb_reader_->lock();
    auto res = this->tb_reader_->Read(this->read_value_);
    this->tb_reader_->unlock();

    // Assert
    ASSERT_EQ(res, true);
    ASSERT_EQ(this->read_value_, this->value_);
}

// Test whether Read for complex data types returns false if memory was not opened before.
TYPED_TEST(SharedMemTimeBaseReaderReadComplexTypeTestFixture, Read_MemoryNotOpened_ReturnFalse) {
    // Act
    auto res = this->tb_reader_->Read(this->struct_);

    // Assert
    ASSERT_EQ(res, false);
}

// Test whether Read for complex data types returns true and correct data in case memory was opened before.
TYPED_TEST(SharedMemTimeBaseReaderReadComplexTypeTestFixture, Read_MemoryOpened_ReturnCorrectData) {
    // Arrange
    auto writer = std::make_unique<SharedMemTimeBaseWriter>(this->shared_mem_name_, true);
    writer->Open();

    this->tb_reader_->Open();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
    memset(&this->struct_, 3, sizeof(this->struct_));
#pragma GCC diagnostic pop

    writer->lock();
    writer->Write(this->struct_);
    writer->unlock();

    // Act
    this->tb_reader_->lock();
    auto res = this->tb_reader_->Read(this->read_data_);
    this->tb_reader_->unlock();

    // Assert
    ASSERT_EQ(res, true);
    ASSERT_EQ(memcmp(&this->read_data_, &this->struct_, sizeof(this->struct_)), 0);
}

// Test whether Read for complex data types returns false if memory was not opened before.
TYPED_TEST(SharedMemTimeBaseReaderReadConfigTypeTestFixture, Read_MemoryNotOpened_ReturnFalse) {
    // Act
    auto res = this->tb_reader_->Read(this->struct_);

    // Assert
    ASSERT_EQ(res, false);
}

// Test whether Read for complex data types returns false if a read fails.
TYPED_TEST(SharedMemTimeBaseReaderReadConfigTypeTestFixture, Read_Fails_ReturnFalse) {
    // Act
    std::size_t &max_size = const_cast<std::size_t &>(this->tb_reader_->max_size_);
    max_size = 10;
    auto res = this->tb_reader_->Read(this->struct_);
    max_size = kSharedMemPageSize;

    // Assert
    ASSERT_EQ(res, false);
}

void fillConfig(TsyncConsumerConfig &cfg) {
    cfg.name = "test";
    cfg.time_slave_config.is_valid = true;
    cfg.time_slave_config.follow_up_timeout_value = std::chrono::milliseconds(42);
    cfg.time_slave_config.time_leap_future_threshold = std::chrono::milliseconds(42);
    cfg.time_slave_config.time_leap_healing_counter = 42U;
    cfg.time_slave_config.time_leap_past_threshold = std::chrono::milliseconds(42);
}

void fillConfig(TsyncProviderConfig &cfg) {
    cfg.name = "test";
    cfg.time_master_config.immediate_resume_time = std::chrono::milliseconds(42);
    cfg.time_master_config.is_system_wide_global_time_master = true;
    cfg.time_master_config.is_valid = true;
    cfg.time_master_config.sync_period = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.allow_provider_rate_correction = true;
    cfg.time_sync_correction_config.num_rate_corrections_per_measurement_duration = 42U;
    cfg.time_sync_correction_config.offset_correction_adaption_interval = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.offset_correction_jump_threshold = std::chrono::milliseconds(42);
    cfg.time_sync_correction_config.rate_deviation_measurement_duration = std::chrono::milliseconds(42);
}

void fillConfig(TsyncEthTimeDomain &cfg) {
    cfg.num_fup_data_id_entries = 16;
    cfg.vlan_priority = 2;
    cfg.message_format = TsyncEthMessageFormat::kIeee8021ASAutosar;
    cfg.is_valid = true;
}

void fillConfig(TsyncTimeDomainConfig &cfg) {
    cfg.domain_id = 12;
    cfg.debounce_time = std::chrono::milliseconds(1000);
    cfg.sync_loss_timeout = std::chrono::milliseconds(1000);
    fillConfig(cfg.eth_time_domain);
    fillConfig(cfg.consumer_config);
    fillConfig(cfg.provider_config);
}

// Test whether Read for complex data types returns true and correct data in case memory was opened before.
// config Read/Writes implictly set the offsets of the structs in shmem, so this must be handled in a
// separate test fixture
TYPED_TEST(SharedMemTimeBaseReaderReadConfigTypeTestFixture, Read_Succeeds) {
    // Arrange
    this->tb_reader_->Open();
    fillConfig(this->struct_);

    SharedMemTimeBaseWriter writer(this->tb_reader_->GetName());
    writer.Open();
    writer.lock();
    auto res = writer.Write(this->struct_);
    writer.unlock();

    ASSERT_TRUE(res);

    // Act
    this->tb_reader_->lock();
    res = this->tb_reader_->Read(this->read_data_);
    this->tb_reader_->unlock();

    // Assert
    ASSERT_TRUE(res);
    ASSERT_EQ(this->read_data_, this->struct_);
}

TYPED_TEST(SharedMemTimeBaseReaderReadConfigTypeTestFixture, Read_MemoryNotOpened_ReturnsError) {
    // Arrange
    auto res = this->tb_reader_->Read(this->read_data_);

    // Assert
    ASSERT_EQ(res, false);
}

TEST(Lib_SharedMemTimeBaseReader, Construction_With_MissingLeadingSlashInName_CorrectsName) {
    // Arrange
    std::string reader_name = "TestReader";
    std::string expected_shm_name = "/" + reader_name;

    // Act
    SharedMemTimeBaseReader reader(reader_name);

    // Assert
    ASSERT_EQ(reader.shm_name_, expected_shm_name);
}

}  // namespace lib_sharedmemtimebasereader_ut
}  // namespace testing
