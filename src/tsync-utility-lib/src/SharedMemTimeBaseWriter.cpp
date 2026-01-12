/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SharedMemTimeBaseWriter.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include "score/time/common/Abort.h"

#include "score/time/utility/SysCalls.h"
#include "score/time/utility/TsyncConfigTypes.h"
#include "score/time/utility/TsyncSharedUtils.h"
#include "SharedMemLayout.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

namespace helpers {
template <typename FIELDTYPE>
std::size_t WriteField(SharedMemTimeBaseWriter& writer, const FIELDTYPE& data);

// template specializations allowing to optimize serialization
// for types without special alignment requirements.
// *Note*: These can only be done at the namespace level, not at the class level.

template <>
std::size_t WriteField<std::string>(SharedMemTimeBaseWriter& writer, const std::string& data) {
    if (writer.GetState() == ITimeBaseAccessor::State::Open) {
        const std::size_t start_pos = writer.GetPosition();
        const std::uint64_t str_len = data.size();
        bool res = writer.Write(str_len);
        if (res) {
            if (str_len == 0u) {
                return sizeof(str_len);
            }

            // add 1 for terminating zero
            uint64_t wsz = str_len;
            static_cast<void>(SafeAdd<std::uint64_t, std::size_t>(wsz, 1u));
            if (writer.Write(data.c_str(), wsz)) {
                std::size_t sz_written = sizeof(str_len);
                static_cast<void>(SafeAdd<std::size_t, std::uint64_t>(sz_written, str_len));
                static_cast<void>(SafeAdd<std::size_t, std::size_t>(sz_written, 1u));

                return sz_written;
            }
        }
        static_cast<void>(writer.SetPosition(start_pos));
    }

    return 0u;
}

template <>
std::size_t WriteField<std::string_view>(SharedMemTimeBaseWriter& writer, const std::string_view& data) {
    if (writer.GetState() == ITimeBaseAccessor::State::Open) {
        std::size_t pos_start = writer.GetPosition();
        std::size_t sz = data.length();
        bool res = writer.Write(static_cast<uint64_t>(sz));
        if (res) {
            if (sz == 0u) {
                return sizeof(sz);
            }

            res = writer.Write(data.data(), sz);
            if (res) {
                std::size_t sz_written = sz;
                // add the size of the length info
                static_cast<void>(SafeAdd<std::size_t, std::size_t>(sz_written, sizeof(sz), writer.GetMaxSize()));
                return sz_written;
            }

            static_cast<void>(writer.SetPosition(pos_start));
        }
    }

    return 0u;
}

}  // namespace helpers

// clang-format off
SharedMemTimeBaseWriter::SharedMemTimeBaseWriter(std::string_view name, bool is_owner) /* noexcept */
    : SharedMemTimeBaseWriter(name, kSharedMemPageSize, is_owner) {
}

// coverity[exn_spec_violation] Size of std::string_view will never exceed std::string.max_size(), so no length error will be thrown
SharedMemTimeBaseWriter::SharedMemTimeBaseWriter(std::string_view name, std::size_t max_size, bool is_owner) /* noexcept */
    : ITimeBaseAccessor{}, ITimeBaseWriter{}, time_base_name_(name), max_size_(max_size), is_owner_(is_owner) {
    shm_name_ = time_base_name_.front() != '/' ? std::string("/") + time_base_name_ : time_base_name_;
}
// clang-format on

// coverity[exn_spec_violation] The string created in Unlink() will never throw a length error
SharedMemTimeBaseWriter::~SharedMemTimeBaseWriter() /* noexcept */ {
    Close();
    Unlink();
}

void SharedMemTimeBaseWriter::Open() /* noexcept */ {
    if (state_ == State::Open) {
        // allow Open() to be called repeatedly with (almost) no runtime penalty
        return;
    }

    auto fileDescriptor = OsShmOpen(shm_name_.c_str(), O_RDWR | (is_owner_ ? O_CREAT : 0),
                                    static_cast<unsigned>(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    if (fileDescriptor < 0) {
        std::stringstream ss;
        ss << "SharedMemTimeBaseWriter::Open() - "
           << "Unable to open shared memory file"
           << " for " << shm_name_.c_str() << " with error " << errno;
        logFatalAndAbort(ss.str().c_str());
    }

    if (is_owner_) {
        if (OsFtruncate(fileDescriptor, static_cast<off_t>(max_size_)) != 0) {
            std::stringstream ss;
            ss << "SharedMemTimeBaseWriter::Open() - "
                << "Unable to set shared memory block size"
                << " for " << shm_name_.c_str() << " with error " << errno;
            logFatalAndAbort(ss.str().c_str());
        }
    }

    void* v_addr = OsMmap(nullptr, max_size_, PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    OsClose(fileDescriptor);
    if (v_addr == MAP_FAILED) {
        std::stringstream ss;
        ss << "SharedMemTimeBaseWriter::Open() - OsMmap failed"
            << " for " << shm_name_.c_str() << " with error " << errno;
        logFatalAndAbort(ss.str().c_str());
    }

    addr_ = static_cast<char*>(v_addr);
    rw_lock_.Open(static_cast<pthread_rwlock_t*>(v_addr), TsyncReadWriteLock::LockMode::Write, is_owner_);

    // not testable, currently the alignment matches
    // if (current_write_offset_ % alignof(std::max_align_t) != 0) {
    //     AlignedSkip<SharedMemTimeBaseWriter, std::max_align_t>(*this);
    // }

    state_ = State::Open;

    if (!(SetPosition(sizeof(TsyncReadWriteLock) + kSharedMemTimebaseDataOffset))) {
        logFatalAndAbort("SharedMemTimeBaseWriter::Open() failed. MaxSize is too small.");
    }
}

void SharedMemTimeBaseWriter::Close() /* noexcept */ {
    if (state_ == ITimeBaseAccessor::State::Open) {
        state_ = ITimeBaseAccessor::State::Closed;
        rw_lock_.Close();
        if (addr_) {
            if (OsMunmap(addr_, max_size_) < 0) {
                std::cerr << "SharedMemTimeBase::Close - munmap failed with error " << errno << "\n";
            } else {
                addr_ = nullptr;
            }
        }
    }
}

std::string_view SharedMemTimeBaseWriter::GetName() const /* noexcept */ {
    return time_base_name_;
}

ITimeBaseAccessor::State SharedMemTimeBaseWriter::GetState() const /* noexcept */ {
    return state_;
}

ITimeBaseAccessor& SharedMemTimeBaseWriter::GetAccessor() /* noexcept */ {
    return *this;
}

bool SharedMemTimeBaseWriter::Write(std::uint8_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::uint16_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::uint32_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::uint64_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::int8_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::int16_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::int32_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(std::int64_t data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(float data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(double data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(const TimestampWithStatus& data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(const VirtualLocalTime& data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::Write(UserData data) /* noexcept */ {
    // We're always writing 4 bytes of UserData
    void* pv = data.data();
    return Write(static_cast<char*>(pv), 4u);
}

bool SharedMemTimeBaseWriter::Write(UserDataView data) /* noexcept */ {
    if (GetState() == ITimeBaseAccessor::State::Closed) {
        return false;
    }

    if (data.size() == 0u) {
        return Write(make_array<std::byte>(0, 0, 0, 0));
    }

    // We're always writing 4 bytes of UserData (UD length + 3 bytes)
    constexpr std::size_t kUserDataMax{3u};

    std::size_t sz = std::min(data.size(), kUserDataMax);
    // clang-format off
    // RULECHECKER_comment(1, 1, check_reinterpret_cast_extended, "This cast is well defined according to the C++ type aliasing rules.", true)
    const char* p = reinterpret_cast<const char*>(data.data());
    // clang-format on
    if (Write(static_cast<uint8_t>(sz)) && Write(p, sz)) {
        return Skip(kUserDataMax - sz);
    }

    return false;
}

bool SharedMemTimeBaseWriter::Write(const SynchronizationStatus& data) /* noexcept */ {
    return (WriteField(data) > 0u);
}

bool SharedMemTimeBaseWriter::WriteDefaults() /* noexcept */ {
    if (state_ == State::Closed) {
        return false;
    }

    TimestampWithStatus ts{SynchronizationStatus::kNotSynchronizedUntilStartup, std::chrono::nanoseconds(0),
                           std::chrono::seconds(0)};
    VirtualLocalTime vt(0);
    UserData ud = {std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};

    return (Write(ts) && Write(vt) && Write(ud));
}

bool SharedMemTimeBaseWriter::Write(const TsyncTimeDomainConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_write_offset_;
        SetPosition(kSharedMemTimebaseDomainConfigOffset);
        res = (SetPosition(kSharedMemTimebaseDomainConfigOffset) && (WriteField(data.eth_time_domain) != 0u) &&
               Write(data.consumer_config) && Write(data.provider_config) && (WriteField(data.sync_loss_timeout) > 0) &&
               (WriteField(data.debounce_time) > 0) && Write(data.domain_id));
        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseWriter::Write(const TsyncConsumerConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_write_offset_;
        res = (SetPosition(kSharedMemTimebaseConsumerConfigOffset) && Write(data.name) &&
               (WriteField(data.time_slave_config) > 0u));
        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseWriter::Write(const TsyncProviderConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_write_offset_;
        res = (SetPosition(kSharedMemTimebaseProviderConfigOffset) && Write(data.name) &&
               (WriteField(data.time_master_config) != 0u) && (WriteField(data.time_sync_correction_config) != 0u));
        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseWriter::Write(const std::string& data) {
    return (helpers::WriteField(*this, data) > 0);
}

bool SharedMemTimeBaseWriter::Write(std::string_view data) {
    return (helpers::WriteField(*this, data) > 0);
}

bool SharedMemTimeBaseWriter::Write(score::cpp::span<std::byte> data) /* noexcept */ {
    if (state_ == State::Open) {
        std::size_t pos{current_write_offset_.load()};
        if (IsInBounds<std::size_t, std::size_t>(pos, data.size(), max_size_)) {
            std::memcpy(addr_ + current_write_offset_, data.data(), data.size());
            return Skip(data.size());
        }
    }
    return false;
}

bool SharedMemTimeBaseWriter::Write(const char* data, std::size_t size) {
    if (state_ == State::Open) {
        if (size == 0u) {
            return true;
        }

        std::size_t pos{current_write_offset_.load()};
        if (IsInBounds<std::size_t, std::size_t>(pos, size, max_size_)) {
            std::memcpy(addr_ + current_write_offset_, data, size);
            return Skip(size);
        }
    }
    return false;
}

bool SharedMemTimeBaseWriter::Skip(std::size_t num_bytes) /* noexcept */ {
    if (state_ == State::Open) {
        auto pos{current_write_offset_.load()};
        if (SafeAdd<std::size_t, std::size_t>(pos, num_bytes, max_size_)) {
            current_write_offset_ = pos;
            return true;
        }
    }

    return false;
}

bool SharedMemTimeBaseWriter::SetPosition(std::size_t pos) /* noexcept */ {
    if (state_ == State::Open && pos < max_size_ && pos >= sizeof(TsyncReadWriteLock)) {
        current_write_offset_ = pos;
        return true;
    }

    return false;
}

std::size_t SharedMemTimeBaseWriter::GetPosition() const /* noexcept */ {
    return current_write_offset_;
}

std::size_t SharedMemTimeBaseWriter::GetMaxSize() const /* noexcept */ {
    return max_size_;
}

void SharedMemTimeBaseWriter::lock() /* noexcept */ {
    if (state_ == State::Open) {
        rw_lock_.lock();
        if (!(SetPosition(sizeof(TsyncReadWriteLock) + kSharedMemTimebaseDataOffset))) {
            logFatalAndAbort("SharedMemTimeBaseWriter::lock(): MaxSize is too small");
        }
    } else {
        logFatalAndAbort("lock called on closed writer");
    }
}
void SharedMemTimeBaseWriter::unlock() /* noexcept */ {
    if (state_ == State::Open) {
        rw_lock_.unlock();
    } else {
        logFatalAndAbort("unlock called on closed writer");
    }
}

void SharedMemTimeBaseWriter::Unlink() {
    if (is_owner_) {
        auto s_len = time_base_name_.size();

        if (s_len > 0U && time_base_name_.back() == '/') {
            s_len--;
        }

        if (OsShmUnlink(time_base_name_.substr(0u, s_len).c_str()) != 0) {
            std::perror("SharedMemTimeBaseWriter::Unlink(): unlinking failed!");
        }
    }
}

}  // namespace time
}  // namespace score
