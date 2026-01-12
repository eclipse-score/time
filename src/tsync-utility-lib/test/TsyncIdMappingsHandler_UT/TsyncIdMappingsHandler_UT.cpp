/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include <array>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "score/time/timestamp.h"
#include <gtest/gtest.h>

#define private public
#include "score/time/utility/TsyncIdMappingsHandler.h"
#undef private

#include "SharedMemTimeBaseReaderMock.h"
#include "SharedMemTimeBaseWriterMock.h"
#include "TimeBaseReaderFactoryMock.h"
#include "TimeBaseWriterFactoryMock.h"

namespace score {
namespace time {
extern std::unique_ptr<::testing::NiceMock<TimeBaseReaderFactoryMock>> reader_factory_mock;
extern std::unique_ptr<::testing::NiceMock<TimeBaseWriterFactoryMock>> writer_factory_mock;
}  // namespace time
}  // namespace score

using score::time::reader_factory_mock;
using score::time::writer_factory_mock;

using namespace score::time;
using ::testing::Return;

constexpr int32_t kNumTimeDomains{4};
const std::string_view kTimeDomainNames[kNumTimeDomains] = {"domain_1", "domain_2", "domain_3", "domain_4"};

const std::string_view kTimeConsumerNames[4] = {"consumer_1", "consumer_2", "consumer_3", "consumer_4"};

const std::string_view kTimeProviderNames[2] = {
    "provider_1",
    "provider_2",
};

static constexpr std::uint64_t kConsumerMagic = 0xabcddcbaabcddcba;
static constexpr std::uint64_t kProviderMagic = 0x1122334444332211;

class TsyncIdMappingsHandlerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();
        uint32_t id = 1;
        for (auto sv : kTimeDomainNames) {
            mappings_handler_.AddDomainMapping(id++, sv);
        }

        mappings_handler_.AddDomainMapping(TsyncIdMappingsHandler::TimeDomainMapping(9, "just_for_coverage"));
    }

    void TearDown() override {
        mappings_handler_.Clear();
        reader_factory_mock.reset();
        writer_factory_mock.reset();
    }

public:
    TsyncIdMappingsHandler mappings_handler_;

    static constexpr const uint32_t invalid_timebase_id_ = 99;
    static constexpr const uint32_t nonexistent_timebase_id_ = 11;
    static constexpr const char* invalid_time_base_name_ = "does not exist";
    static constexpr size_t invalid_id_mapping_no_ = 5;
};

class TsyncIdMappingsHandlerDeathFixture : public ::testing::Test {
protected:
    void SetUp() override {
        reader_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseReaderFactoryMock>>();
        writer_factory_mock = std::make_unique<::testing::NiceMock<TimeBaseWriterFactoryMock>>();

        mh_ = std::make_unique<TsyncIdMappingsHandler>();
        mh_->reader_ = TimeBaseReaderFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize);
        mh_->writer_ = TimeBaseWriterFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize);

        SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());
        ::testing::Mock::AllowLeak(reader_mock);
        // install abort handler for our death tests
        //!! ara::core::SetAbortHandler(&AbortHandler);
    }

    void TearDown() override {
        reader_factory_mock.reset();
        writer_factory_mock.reset();
        mh_.reset();
        //!! ara::core::SetAbortHandler(nullptr);
    }

    void AddMappings() {
        uint32_t id = 1;
        for (auto sv : kTimeDomainNames) {
            mh_->AddDomainMapping(id++, sv);
        }
        mh_->AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]);
    }

    static void AbortHandler() noexcept {
        // the mocks have to be reset here, otherwise the expectations for our death tests
        // will never be met/evaluated.
        reader_factory_mock.reset();
        writer_factory_mock.reset();
        mh_.reset();
        std::exit(EXIT_CODE);
    }

    // needs to be deleted in the abort handler so the set up expectations
    // are applied
    static std::unique_ptr<TsyncIdMappingsHandler> mh_;
    static constexpr const int32_t EXIT_CODE = 42;
};

std::unique_ptr<TsyncIdMappingsHandler> TsyncIdMappingsHandlerDeathFixture::mh_{};

namespace testing {
namespace lib_tsyncidmappingshandler_ut {

TEST_F(TsyncIdMappingsHandlerFixture, Ctor_Succeeds) {
    // arrange
    TsyncIdMappingsHandler mh;

    // assert
    ASSERT_FALSE(mh.reader_);
    ASSERT_FALSE(mh.writer_);
    ASSERT_TRUE(mh.time_domains_.empty());
    ASSERT_TRUE(mh.provider_to_time_domain_mappings_.empty());
    ASSERT_TRUE(mh.consumer_to_time_domain_mappings_.empty());
}

TEST_F(TsyncIdMappingsHandlerFixture, MoveCtor_Succeeds) {
    // arrange
    TsyncIdMappingsHandler mh1;
    TsyncIdMappingsHandler mh2{std::move(mh1)};

    // assert
    ASSERT_FALSE(mh2.reader_);
    ASSERT_FALSE(mh2.writer_);
    ASSERT_TRUE(mh2.time_domains_.empty());
    ASSERT_TRUE(mh2.provider_to_time_domain_mappings_.empty());
    ASSERT_TRUE(mh2.consumer_to_time_domain_mappings_.empty());
}

TEST_F(TsyncIdMappingsHandlerFixture, MoveAssignment_Succeeds) {
    // arrange
    TsyncIdMappingsHandler mh1;

    // act
    mh1 = TsyncIdMappingsHandler();

    // assert
    TsyncIdMappingsHandler mh2;

    ASSERT_EQ(mh1.time_domains_.size(), mh2.time_domains_.size());
    ASSERT_EQ(mh1.consumer_to_time_domain_mappings_.size(), mh2.consumer_to_time_domain_mappings_.size());
    ASSERT_EQ(mh1.provider_to_time_domain_mappings_.size(), mh2.provider_to_time_domain_mappings_.size());
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_OnWriteNumberOfDomainsError_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            EXPECT_CALL(*writer_mock, Write(testing::An<uint32_t>())).WillOnce(Return(false));

            // act
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing time domain mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_OnWriteDommainMappingError_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            AddMappings();
            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            InSequence seq;
            EXPECT_CALL(*writer_mock, Write(testing::An<uint32_t>())).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::An<uint32_t>())).WillOnce(Return(false));

            // act
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing time domain mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_OnTooManyConsumerMappings_Aborts) {
    auto fn = [] {
        std::array<std::string, 129> consumer_names{};
        std::array<std::string_view, 129> consumer_names_views{};
        for (std::uint32_t i = 0; i < consumer_names.size(); ++i) {
            consumer_names[i] = std::to_string(i).c_str();
            consumer_names_views[i] = consumer_names[i];

            mh_->consumer_to_time_domain_mappings_.insert({consumer_names_views[i], {1u, kTimeDomainNames[0u]}});
        }

        // act
        mh_->CommitMappingsToSharedMemory();
    };

    ASSERT_EXIT(
        {  // arrange
            AddMappings();
            // act
            fn();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Number of consumer mappings has exceeded the maximum permissible value");
}

TEST_F(TsyncIdMappingsHandlerFixture, CommitMappingsToSharedMemory_Succeeds) {
    // act
    mappings_handler_.CommitMappingsToSharedMemory();
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_WithTooManyTimeDomains_Aborts) {
    ASSERT_EXIT(
        {
            std::vector<std::string> domain_names;
            std::string_view base_name{"MyDomain"};

            // create domains
            for (std::size_t i = 0; i <= TsyncIdMappingsHandler::kMaxNumberOfTimeDomains; ++i) {
                domain_names.push_back(std::string(base_name.data()) + std::to_string(i).c_str());
                mh_->time_domains_.insert(std::make_pair(i + 1, std::string_view((--domain_names.end())->c_str())));
            }

            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Number of time domains has exceeded");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_WithDomainWriteError_Aborts) {
    ASSERT_EXIT(
        {
            AddMappings();

            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            EXPECT_CALL(*writer_mock, GetState()).WillOnce(Return(ITimeBaseAccessor::State::Open));
            EXPECT_CALL(*writer_mock,
                        Write(testing::Matcher<std::uint32_t>(static_cast<uint32_t>(mh_->time_domains_.size()))))
                .WillOnce(Return(true));
            // 1 is the id of the first time domain
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::uint32_t>(mh_->time_domains_.begin()->first)))
                .WillOnce(Return(false));
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing time domain mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture,
       CommitMappingsToSharedMemory_WithConsumerMappingStartMagicWriteError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            EXPECT_CALL(*writer_mock, GetState()).WillOnce(Return(ITimeBaseAccessor::State::Open));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::uint64_t>(0xabcddcbaabcddcba)))
                .WillOnce(Return(false));
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing consumer mappings header");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_WithConsumerMappingWriteError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());
            AddMappings();
            mh_->AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]);

            EXPECT_CALL(*writer_mock, GetState()).WillOnce(Return(ITimeBaseAccessor::State::Open));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeDomainNames[0]))).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeDomainNames[1]))).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeDomainNames[2]))).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeDomainNames[3]))).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(std::string_view("just_for_coverage"))))
                .WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeConsumerNames[0])))
                .WillOnce(Return(false));
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing consumer mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture,
       CommitMappingsToSharedMemory_WithProviderMappingStartMagicWriteError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            EXPECT_CALL(*writer_mock, GetState()).WillOnce(Return(ITimeBaseAccessor::State::Open));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::uint64_t>(kConsumerMagic))).WillOnce(Return(true));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::uint64_t>(kProviderMagic))).WillOnce(Return(false));
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing provider mappings header");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, CommitMappingsToSharedMemory_WithProviderMappingWriteError_Aborts) {
    ASSERT_EXIT(
        {
            AddMappings();

            SharedMemTimeBaseWriterMock* writer_mock = static_cast<SharedMemTimeBaseWriterMock*>(mh_->writer_.get());

            EXPECT_CALL(*writer_mock, GetState()).WillOnce(Return(ITimeBaseAccessor::State::Open));
            EXPECT_CALL(*writer_mock, Write(testing::Matcher<std::string_view>(kTimeProviderNames[0])))
                .WillOnce(Return(false));
            mh_->CommitMappingsToSharedMemory();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error writing provider mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, DoSharedMemoryRead_WithProviderMagicReadError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            std::uint32_t num_domains = 1u;
            std::uint32_t domain_id = 1u;
            std::uint32_t num_consumer_mappings = 0u;
            std::uint64_t corrupt_provider_magic = 0u;

            // read of configured time domains (number id, name)
            testing::InSequence seq;
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeDomainNames[0]), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kConsumerMagic), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(num_consumer_mappings), ::testing::Return(true)));
            // read of provider magic fails
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(corrupt_provider_magic), ::testing::Return(true)));
            mh_->DoSharedMemoryRead();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error reading start of provider mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, DoSharedMemoryRead_OnAddProviderToDomainMappingError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            std::uint32_t num_domains = 1u;
            std::uint32_t domain_id = 1u;
            std::uint32_t invalid_domain_id = 42u;
            std::uint32_t num_consumer_mappings = 0u;
            std::uint32_t num_provider_mappings = 1u;

            // read of configured time domains (number id, name)
            testing::InSequence seq;
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeDomainNames[0]), ::testing::Return(true)));
            // read of consumer magic
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kConsumerMagic), ::testing::Return(true)));
            // read of number of consumer mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(num_consumer_mappings), ::testing::Return(true)));
            // read of provider magic
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kProviderMagic), ::testing::Return(true)));
            // read of number of provider mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(num_provider_mappings), ::testing::Return(true)));
            // add provider mapping to non-existing domain
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(kTimeProviderNames[0]), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(invalid_domain_id), ::testing::Return(true)));

            mh_->DoSharedMemoryRead();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error adding provider");
}

TEST_F(TsyncIdMappingsHandlerFixture, DoSharedMemoryRead_Succeeds) {
    TsyncIdMappingsHandler mh;
    mh.reader_ = TimeBaseReaderFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize);
    SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh.reader_.get());

    std::uint32_t num_domains = 1u;
    std::uint32_t domain_id = 1u;
    std::uint32_t num_consumer_mappings = 1u;
    std::uint32_t num_provider_mappings = 1u;

    // read of configured time domains (number id, name)
    testing::InSequence seq;
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeDomainNames[0]), ::testing::Return(true)));
    // read of consumer magic
    EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kConsumerMagic), ::testing::Return(true)));
    // read of number of consumer mappings
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_consumer_mappings), ::testing::Return(true)));
    // read of consumer mapping
    EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeConsumerNames[0]), ::testing::Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));
    // read of provider magic
    EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kProviderMagic), ::testing::Return(true)));
    // read of number of provider mappings
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_provider_mappings), ::testing::Return(true)));
    // read of provider mapping
    EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeProviderNames[0]), ::testing::Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));

    mh.DoSharedMemoryRead();
    SUCCEED();
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, DoSharedMemoryRead_WithConsumerMagicReadError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());
            std::uint32_t num_domains = 1;
            std::uint32_t domain_id = 1;
            std::uint64_t corrupt_magic = 0;

            // read of configured time domains (number, id, name)
            testing::InSequence seq;
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(domain_id), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeDomainNames[0]), ::testing::Return(true)));
            // failure to read consumer magic
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(corrupt_magic), ::testing::Return(true)));
            // number of consumer mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));

            mh_->DoSharedMemoryRead();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error reading start of consumer mappings");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, DoSharedMemoryRead_WithInvalidDomainIdReadError_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            std::uint32_t num_domains = 1;
            std::uint32_t num_consumer_mappings = 0;
            std::uint32_t num_provider_mappings = 1;
            std::uint32_t valid_domain_id = 5;
            std::uint32_t invalid_domain_id = 77;

            testing::InSequence seq;
            // number of domain mapping
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));
            // domain id
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(valid_domain_id), ::testing::Return(true)));
            // domain name
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kTimeDomainNames[0]), ::testing::Return(true)));
            // consumer mappings magic
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kConsumerMagic), ::testing::Return(true)));
            // number of consumer mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(num_consumer_mappings), ::testing::Return(true)));
            // provider mappings magic
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(kProviderMagic), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(num_provider_mappings), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(
                    ::testing::DoAll(::testing::SetArgReferee<0>(kTimeProviderNames[0]), ::testing::Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(invalid_domain_id), ::testing::Return(true)));
            mh_->DoSharedMemoryRead();
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error adding provider");
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainId_WithValidDomainName_Succeeds) {
    // arranged through fixture

    // act
    auto ret = mappings_handler_.GetDomainId(kTimeDomainNames[0]);

    // assert
    ASSERT_EQ(ret, 1U);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainId_WithNonexistentDomainName_Fails) {
    // arranged through test-fixture

    // act
    auto ret = mappings_handler_.GetDomainId(invalid_time_base_name_);

    // assert
    ASSERT_FALSE(ret);
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainId_OnNumMappingsIsZero_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());
            std::uint32_t num_domains = 0u;

            EXPECT_CALL(*reader_mock, Read(::testing::An<uint32_t&>()))
                .WillOnce(DoAll(SetArgReferee<0>(num_domains), Return(true)));

            // act
            auto ret = mh_->GetDomainId("XYZ");
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainId_OnShMemReadError_Aborts) {
    ASSERT_EXIT(
        {
            auto reader_mock = mh_->reader_.get();
            auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock);
            EXPECT_CALL(*raw_reader_mock, Read(::testing::An<uint32_t&>())).WillOnce(::testing::Return(false));

            // act
            mh_->GetDomainId("XYZ");
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainName_WithValidDomainId_Succeeds) {
    // arranged through fixture

    // act
    auto ret = mappings_handler_.GetDomainName(1);

    // assert
    ASSERT_EQ(ret, kTimeDomainNames[0]);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainName_WithInvalidTimeBaseId_Fails) {
    // arranged through fixture

    // act
    auto ret = mappings_handler_.GetDomainName(invalid_timebase_id_);

    // assert
    ASSERT_FALSE(ret);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainName_WithNonexistentTimeBaseId_Fails) {
    // arranged through fixture

    // act
    auto ret = mappings_handler_.GetDomainName(nonexistent_timebase_id_);

    // assert
    ASSERT_FALSE(ret);
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, DoSharedMemoryRead_OnNumMappingsIsZero_Aborts) {
    ASSERT_EXIT(
        {
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());
            std::uint32_t num_domains = 0u;

            EXPECT_CALL(*reader_mock, Read(::testing::An<uint32_t&>()))
                .WillOnce(DoAll(SetArgReferee<0>(num_domains), Return(true)));
            // act
            mh_->GetDomainName(1);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerFixture, AddDomainMapping_WithValidParams_Succeeds) {
    // arrange
    TsyncIdMappingsHandler mappings_handler;

    // act AddDomainMapping increments num_mappings_ by one
    auto ret = mappings_handler.AddDomainMapping(1, kTimeDomainNames[0]);

    // assert
    ASSERT_TRUE(ret);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddDomainMapping_WithInvalidId_Failed) {
    // arrange
    TsyncIdMappingsHandler mappings_handler;

    // act AddDomainMapping increments num_mappings_ by one
    auto ret = mappings_handler.AddDomainMapping(100, kTimeDomainNames[0]);

    // assert
    ASSERT_FALSE(ret);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddDomainMapping_WithDomaiHaveInvalidId_Failed) {
    // arrange
    TsyncIdMappingsHandler mappings_handler;
    TsyncIdMappingsHandler::TimeDomainMapping domain_mapping;
    domain_mapping.first = 100;
    domain_mapping.second = "Invalid_domain";

    // act AddDomainMapping increments num_mappings_ by one
    auto ret = mappings_handler.AddDomainMapping(domain_mapping);

    // assert
    ASSERT_FALSE(ret);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainName_WithReadFromSharedMemory_Succeeds) {
    // arrange
    TsyncIdMappingsHandler mh;
    mh.reader_ = TimeBaseReaderFactory::Create(kIdMappingsShmemFileName, kIdMappingsShmemSize);
    SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh.reader_.get());

    uint32_t domain_count_and_id = 1;
    uint32_t mapping_count = 0;
    InSequence seq;
    EXPECT_CALL(*reader_mock, Open()).Times(1);
    EXPECT_CALL(*reader_mock, lock()).Times(1);
    // read of domain count, domain id & name
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[0]), Return(true)));
    // read of consumer magic and number of consumer mappings
    EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kConsumerMagic), Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(mapping_count), Return(true)));
    // read of provider magic and number of provider mappings
    EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kProviderMagic), Return(true)));
    EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
        .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(mapping_count), Return(true)));

    EXPECT_CALL(*reader_mock, unlock()).Times(1);

    auto res = mh.GetDomainName(1);

    ASSERT_TRUE(res);
    ASSERT_EQ(kTimeDomainNames[0], *res);
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainName_OnDomainMappingReadFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            uint32_t count_and_id = 1;
            EXPECT_CALL(*reader_mock, Open()).Times(1);
            EXPECT_CALL(*reader_mock, lock()).Times(1);
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillRepeatedly(::testing::DoAll(testing::SetArgReferee<0>(count_and_id), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>())).WillOnce(Return(false));

            // act
            mh_->GetDomainName(1);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error reading domain mapping");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainName_OnConsumerMappingReadFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            uint32_t domain_count_and_id = 1;
            uint32_t consumer_count = 2;
            InSequence seq;
            EXPECT_CALL(*reader_mock, Open()).Times(1);
            EXPECT_CALL(*reader_mock, lock()).Times(1);
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[0]), Return(true)));
            // read of consumer magic and number of consumer mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kConsumerMagic), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(consumer_count), Return(true)));
            // failure to read consumer mapping
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>())).WillOnce(Return(false));

            // act
            mh_->GetDomainName(1);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error reading consumer mapping");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainName_OnProviderMappingReadFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            SharedMemTimeBaseReaderMock* reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(mh_->reader_.get());

            uint32_t domain_count_and_id = 1;
            uint32_t consumer_count = 1;
            uint32_t provider_count = 1;
            InSequence seq;
            EXPECT_CALL(*reader_mock, Open()).Times(1);
            EXPECT_CALL(*reader_mock, lock()).Times(1);
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeDomainNames[0]), Return(true)));
            // read of consumer magic and number of consumer mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kConsumerMagic), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(consumer_count), Return(true)));
            // read of consumer mapping
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kTimeConsumerNames[0]), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(domain_count_and_id), Return(true)));
            // read of provider magic and number of provider mappings
            EXPECT_CALL(*reader_mock, Read(testing::An<uint64_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(kProviderMagic), Return(true)));
            EXPECT_CALL(*reader_mock, Read(testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(testing::SetArgReferee<0>(provider_count), Return(true)));
            // failure to read provider mapping
            EXPECT_CALL(*reader_mock, Read(testing::An<std::string_view&>())).WillOnce(Return(false));
            // act
            mh_->GetDomainName(1);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "Error reading provider mapping");
}

TEST_F(TsyncIdMappingsHandlerFixture, IterateTimeDomains_Succeeds) {
    int32_t cnt = 0;
    for (const auto& it : mappings_handler_) {
        (void)it;
        ++cnt;
    }

    // subtract 1 for the "invalid" time domain
    ASSERT_EQ(cnt - 1, kNumTimeDomains);
}

TEST_F(TsyncIdMappingsHandlerFixture, IsEmpty_OnEmpty_Succeeds) {
    TsyncIdMappingsHandler mh;
    ASSERT_TRUE(mh.IsEmpty());
}

TEST_F(TsyncIdMappingsHandlerFixture, IterateConstTimeDomains_Succeeds) {
    const auto& cmh = mappings_handler_;

    int32_t cnt = 0;
    for (auto it = cmh.cbegin(); it != cmh.cend(); ++it) {
        ++cnt;
    }

    // subtract 1 for the "invalid" time domain
    ASSERT_EQ(cnt - 1, kNumTimeDomains);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddConsumerToDomain_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(2U, kTimeConsumerNames[1]));

    // for coverage
    mappings_handler_.DumpMappings();
}

TEST_F(TsyncIdMappingsHandlerFixture, AddConsumerToDomain_SameConsumerTwice_Fails) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_FALSE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
}

TEST_F(TsyncIdMappingsHandlerFixture, AddConsumerToDomain_WithInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.AddConsumerToDomain("bla", kTimeConsumerNames[0]));
    ASSERT_FALSE(mappings_handler_.AddConsumerToDomain(17U, kTimeConsumerNames[1]));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainNameForConsumer_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_EQ(*(mappings_handler_.GetDomainNameForConsumer(kTimeConsumerNames[0])), kTimeDomainNames[0]);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainNameForConsumer_WithInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.GetDomainNameForConsumer("bla"));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainNameForConsumer_WithInvalidConsumer_Fails) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_FALSE(mappings_handler_.GetDomainNameForConsumer("bla"));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainIdForConsumer_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_EQ(*(mappings_handler_.GetDomainIdForConsumer(kTimeConsumerNames[0])), 1U);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainIdForConsumer_WithInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.GetDomainIdForConsumer("bla"));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainIdForConsumer_WithInvalidConsumer_Fails) {
    ASSERT_TRUE(mappings_handler_.AddConsumerToDomain(kTimeDomainNames[0], kTimeConsumerNames[0]));
    ASSERT_FALSE(mappings_handler_.GetDomainIdForConsumer("bla"));
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomain_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(2U, kTimeProviderNames[1]));

    // for coverage
    mappings_handler_.DumpMappings();
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomain_MoreThanOneProvider_Fails) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_FALSE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[1]));
    ASSERT_FALSE(mappings_handler_.AddProviderToDomain(1, kTimeProviderNames[1]));
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomain_WithInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.AddProviderToDomain("bla", kTimeProviderNames[0]));
    ASSERT_FALSE(mappings_handler_.AddProviderToDomain(17U, kTimeProviderNames[1]));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainNameForProvider_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_EQ(*(mappings_handler_.GetDomainNameForProvider(kTimeProviderNames[0])), kTimeDomainNames[0]);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainNameForProvider_WithInvalidProvider_Fails) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_FALSE(mappings_handler_.GetDomainNameForProvider("bla"));
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainIdForProvider_Succeeds) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_EQ(*(mappings_handler_.GetDomainIdForProvider(kTimeProviderNames[0])), 1U);
}

TEST_F(TsyncIdMappingsHandlerFixture, GetDomainIdForProvider_WithInvalidProvider_Fails) {
    ASSERT_TRUE(mappings_handler_.AddProviderToDomain(kTimeDomainNames[0], kTimeProviderNames[0]));
    ASSERT_FALSE(mappings_handler_.GetDomainIdForProvider("bla"));
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetDomainIdForConsumer_OnZeroTimeDomains_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            uint32_t num_domains = 0;

            // arrange
            auto reader_mock = mh_->reader_.get();
            auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock);
            EXPECT_CALL(*raw_reader_mock, Read(::testing::An<uint32_t&>()))
                .WillOnce(::testing::DoAll(::testing::SetArgReferee<0>(num_domains), ::testing::Return(true)));

            // act
            mh_->GetDomainIdForConsumer(kTimeConsumerNames[0]);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetProviderForDomain_OnReadNumberOfTimeDomainsFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto reader_mock = mh_->reader_.get();
            auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock);
            EXPECT_CALL(*raw_reader_mock, Read(::testing::An<uint32_t&>())).WillOnce(::testing::Return(false));

            // act
            auto r = mh_->GetProviderForDomain(kTimeDomainNames[0]);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerDeathFixture, GetProviderForDomainId_OnDoSharedMemoryReadFailure_Aborts) {
    ASSERT_EXIT(
        {
            // arrange
            auto reader_mock = mh_->reader_.get();
            auto raw_reader_mock = static_cast<SharedMemTimeBaseReaderMock*>(reader_mock);
            EXPECT_CALL(*raw_reader_mock, Read(::testing::An<uint32_t&>())).WillOnce(::testing::Return(false));

            // act
            auto r = mh_->GetProviderForDomain(1);
        },
        ::testing::ExitedWithCode(EXIT_CODE), "No domain mappings found");
}

TEST_F(TsyncIdMappingsHandlerFixture, GetProviderForDomainId_WithInvalidDomainId_Fails) {
    // act
    auto r = mappings_handler_.GetProviderForDomain(invalid_timebase_id_);

    // assert
    ASSERT_FALSE(r);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddConsumerToDomainMapping_OnEmpty_Fails) {
    // arrange
    TsyncIdMappingsHandler mh;

    ASSERT_FALSE(mh.reader_);
    ASSERT_FALSE(mh.writer_);
    ASSERT_TRUE(mh.time_domains_.empty());
    ASSERT_TRUE(mh.provider_to_time_domain_mappings_.empty());
    ASSERT_TRUE(mh.consumer_to_time_domain_mappings_.empty());
    // act
    bool res = mh.AddConsumerToDomainMapping(1, std::string_view("domain_1"));
    // assert
    EXPECT_FALSE(res);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomainMapping_OnEmpty_Fails) {
    // arrange
    TsyncIdMappingsHandler mh;

    ASSERT_FALSE(mh.reader_);
    ASSERT_FALSE(mh.writer_);
    ASSERT_TRUE(mh.time_domains_.empty());
    ASSERT_TRUE(mh.provider_to_time_domain_mappings_.empty());
    ASSERT_TRUE(mh.consumer_to_time_domain_mappings_.empty());
    // act
    bool res = mh.AddProviderToDomainMapping(1, std::string_view("domain_1"));
    // assert
    EXPECT_FALSE(res);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomainMapping_Succeeds) {
    // arrange
    // act
    bool res = mappings_handler_.AddProviderToDomainMapping(1, std::string_view("test_provider"));
    // assert
    EXPECT_TRUE(res);
}

TEST_F(TsyncIdMappingsHandlerFixture, AddConsumerToDomainMapping_OnInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.AddConsumerToDomainMapping(17U, kTimeConsumerNames[1]));
}

TEST_F(TsyncIdMappingsHandlerFixture, AddProviderToDomainMapping_OnInvalidDomain_Fails) {
    ASSERT_FALSE(mappings_handler_.AddProviderToDomainMapping(17U, kTimeProviderNames[1]));
}

}  // namespace lib_tsyncidmappingshandler_ut
}  // namespace testing
