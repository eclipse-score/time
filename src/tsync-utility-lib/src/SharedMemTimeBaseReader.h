/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEREADER_H_
#define SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEREADER_H_

#include <atomic>
#include <cstring>

#include "score/time/utility/ITimeBaseAccessor.h"
#include "score/time/utility/ITimeBaseReader.h"
#include "TsyncReadWriteLock.h"

namespace score {
namespace time {

// clang-format off
// RULECHECKER_comment(1, 1, check_multiple_non_interface_bases, "wi 48449 - Inheriting a class with special member functions is not considered an Interface class.", false)
class SharedMemTimeBaseReader final : public ITimeBaseAccessor, public ITimeBaseReader {
    // clang-format on
public:
    SharedMemTimeBaseReader(std::string_view name) noexcept;
    SharedMemTimeBaseReader(std::string_view name, std::size_t max_size) noexcept;
    ~SharedMemTimeBaseReader() noexcept;
    SharedMemTimeBaseReader() = delete;
    SharedMemTimeBaseReader(const SharedMemTimeBaseReader&) = delete;
    SharedMemTimeBaseReader& operator=(const SharedMemTimeBaseReader&) = delete;

    // ITimeBaseReader
    ITimeBaseAccessor& GetAccessor() /* noexcept */ override;

    bool Read(std::uint8_t& data) /* noexcept */ override;
    bool Read(std::uint16_t& data) /* noexcept */ override;
    bool Read(std::uint32_t& data) /* noexcept */ override;
    bool Read(std::uint64_t& data) /* noexcept */ override;
    bool Read(std::int8_t& data) /* noexcept */ override;
    bool Read(std::int16_t& data) /* noexcept */ override;
    bool Read(std::int32_t& data) /* noexcept */ override;
    bool Read(std::int64_t& data) /* noexcept */ override;
    bool Read(float& data) /* noexcept */ override;
    bool Read(double& data) /* noexcept */ override;
    bool Read(std::string& data) /* noexcept */ override;
    bool Read(std::string_view& data) /* noexcept */ override;
    bool Read(const char** p, std::size_t num_bytes) /* noexcept */ override;

    // Function to remove dependency from ptp-lib
    bool Read(TimestampWithStatus& data) /* noexcept */ override;
    bool Read(VirtualLocalTime& data) /* noexcept */ override;
    bool Read(UserDataView& data) /* noexcept */ override;
    bool Read(SynchronizationStatus& data) /* noexcept */ override;

    bool Read(TsyncTimeDomainConfig& data) /* noexcept */ override;
    bool Read(TsyncConsumerConfig& data) /* noexcept */ override;
    bool Read(TsyncProviderConfig& data) /* noexcept */ override;

    bool Skip(std::size_t num_bytes) /* noexcept */ override;
    bool SetPosition(std::size_t pos) /* noexcept */ override;
    std::size_t GetPosition() const /* noexcept */ override;
    std::size_t GetMaxSize() const /* noexcept */ override;

    void lock() /* noexcept */ override;
    void unlock() /* noexcept */ override;

    // ITimeBaseAccessor
    void Open() /* noexcept */ override;
    void Close() /* noexcept */ override;
    std::string_view GetName() const /* noexcept */ override;
    State GetState() const /* noexcept */ override;

private:
    const std::string time_base_name_{};
    std::string shm_name_{};
    TsyncReadWriteLock rw_lock_{};
    const std::size_t max_size_{};
    char* addr_ = nullptr;
    char* addr_read_only_ = nullptr;
    std::atomic<std::size_t> current_read_offset_{0};
    std::atomic<ITimeBaseAccessor::State> state_{State::Closed};

    template <typename FIELDTYPE>
    std::size_t ReadField(FIELDTYPE& data) /* noexcept */ {
        if (GetState() == ITimeBaseAccessor::State::Closed) {
            return 0u;
        }
        bool res = Align<SharedMemTimeBaseReader, FIELDTYPE>(*this);

        if (!res) {
            return 0u;
        }

        std::size_t pos = GetPosition();

        std::memcpy(&data, addr_read_only_ + pos, sizeof(FIELDTYPE));
        static_cast<void>(SetPosition(pos + sizeof(FIELDTYPE)));
        return sizeof(FIELDTYPE);
    }
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEREADER_H_
