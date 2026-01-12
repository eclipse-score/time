/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEWRITER_H_
#define SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEWRITER_H_

#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <streambuf>

#include "score/time/utility/ITimeBaseAccessor.h"
#include "score/time/utility/ITimeBaseWriter.h"
#include "TsyncReadWriteLock.h"

namespace score {
namespace time {

// clang-format off
// RULECHECKER_comment(1, 1, check_multiple_non_interface_bases, "wi 48449 - Inheriting a class with special member functions is not considered an Interface class.", false)
class SharedMemTimeBaseWriter final : public ITimeBaseAccessor, public ITimeBaseWriter {
    // clang-format on
public:
    SharedMemTimeBaseWriter(std::string_view name, bool is_owner = false) /* noexcept*/;
    SharedMemTimeBaseWriter(std::string_view name, std::size_t max_size, bool is_owner = false) /* noexcept*/;
    ~SharedMemTimeBaseWriter() /* noexcept */;
    SharedMemTimeBaseWriter() = delete;
    SharedMemTimeBaseWriter(const SharedMemTimeBaseWriter&) = delete;
    SharedMemTimeBaseWriter& operator=(const SharedMemTimeBaseWriter&) = delete;

    // ITimeBaseWriter
    ITimeBaseAccessor& GetAccessor() /* noexcept */ override;

    bool Write(std::uint8_t data) /* noexcept */ override;
    bool Write(std::uint16_t data) /* noexcept */ override;
    bool Write(std::uint32_t data) /* noexcept */ override;
    bool Write(std::uint64_t data) /* noexcept */ override;
    bool Write(std::int8_t data) /* noexcept */ override;
    bool Write(std::int16_t data) /* noexcept */ override;
    bool Write(std::int32_t data) /* noexcept */ override;
    bool Write(std::int64_t data) /* noexcept */ override;
    bool Write(float data) /* noexcept */ override;
    bool Write(double data) /* noexcept */ override;
    bool Write(const std::string& data) /* noexcept */ override;
    bool Write(std::string_view data) /* noexcept */ override;
    bool Write(score::cpp::span<std::byte> data) /* noexcept */ override;
    // Function to remove dependency on ptp-lib**********************************
    bool Write(const TimestampWithStatus& data) /* noexcept */ override;
    bool Write(const VirtualLocalTime& data) /* noexcept */ override;
    bool Write(UserData data) /* noexcept */ override;
    bool Write(UserDataView data) /* noexcept */ override;
    bool Write(const SynchronizationStatus& data) /* noexcept */ override;

    bool WriteDefaults() /* noexcept */ override;

    bool Write(const TsyncTimeDomainConfig& data) /* noexcept */ override;
    bool Write(const TsyncConsumerConfig& data) /* noexcept */ override;
    bool Write(const TsyncProviderConfig& data) /* noexcept */ override;

    bool Write(const char* data, std::size_t size) override;

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
    template <typename FIELDTYPE>
    std::size_t WriteField(const FIELDTYPE& data) /* noexcept */ {
        if (GetState() == ITimeBaseAccessor::State::Closed) {
            return 0u;
        }

        bool res = Align<SharedMemTimeBaseWriter, FIELDTYPE>(*this);

        if (!res) {
            return 0u;
        }

        std::size_t pos = GetPosition();

        std::memcpy(addr_ + pos, &data, sizeof(FIELDTYPE));
        static_cast<void>(SetPosition(pos + sizeof(FIELDTYPE)));
        return sizeof(FIELDTYPE);
    }

    void Unlink();

   private:
    const std::string time_base_name_{};
    std::string shm_name_{};
    TsyncReadWriteLock rw_lock_{};
    const std::size_t max_size_{};
    char* addr_ = nullptr;
    std::atomic<std::size_t> current_write_offset_{0};
    std::atomic<ITimeBaseAccessor::State> state_{State::Closed};
    bool is_owner_{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_SHAREDMEMTIMEBASEWRITER_H_
