/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "SharedMemTimeBaseReader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <mutex>
#include <sstream>
#include <thread>

#include "score/time/common/Abort.h"

#include "SharedMemLayout.h"
#include "SysCalls.h"
#include "TsyncConfigTypes.h"
#include "TsyncSharedUtils.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

namespace helpers {
template <typename FIELDTYPE>
std::size_t ReadField(SharedMemTimeBaseReader& reader, FIELDTYPE& data) /* noexcept */;

// template specializations allowing to optimize deserialization
// for types without special alignment requirements.
// *Note*: These can only be done at the namespace level, not at the class level.
template <>
std::size_t ReadField<std::string>(SharedMemTimeBaseReader& reader, std::string& data) /* noexcept */ {
    if (reader.GetState() == ITimeBaseAccessor::State::Closed) {
        return 0u;
    }

    std::size_t pos_start = reader.GetPosition();

    // read the string size
    std::uint64_t sz;
    bool res = reader.Read(sz);

    if (!res) {
        return 0;
    }

    if (sz == 0u) {
        data.clear();
        return sizeof(sz);
    }

    // add one for terminating zero
    if (!SafeAdd<std::uint64_t, std::size_t>(sz, 1u, reader.GetMaxSize())) {
        static_cast<void>(reader.SetPosition(pos_start));
        return 0u;
    }

    // zero copy read of the string data
    const char* p = nullptr;
    if (!reader.Read(&p, sz)) {
        static_cast<void>(reader.SetPosition(pos_start));
        return 0u;
    }

    // assign the raw string data & verify the string length
    data = p;
    if (data.length() != sz - 1) {
        static_cast<void>(reader.SetPosition(pos_start));
        return 0u;
    }

    std::size_t bytes_read = reader.GetPosition();
    static_cast<void>(SafeSub(bytes_read, pos_start));
    return bytes_read;
}

template <>
std::size_t ReadField<std::string_view>(SharedMemTimeBaseReader& reader, std::string_view& data) /* noexcept */ {
    if (reader.GetState() == ITimeBaseAccessor::State::Closed) {
        return 0u;
    }

    std::size_t pos_start = reader.GetPosition();

    // read the stringview size
    std::uint64_t sz;
    if (!reader.Read(sz)) {
        return 0u;
    }

    if (sz == 0u) {
        data = std::string_view();
        return sizeof(sz);
    }

    std::size_t end_pos = reader.GetPosition();
    if (!SafeAdd<std::size_t, std::uint64_t>(end_pos, sz, reader.GetMaxSize())) {
        static_cast<void>(reader.SetPosition(pos_start));
        return 0u;
    }

    // zero copy read of the string data
    const char* p = nullptr;
    (void)reader.Read(&p, sz);

    // assign the raw string data
    data = std::string_view(p, sz);

    std::size_t bytes_read = reader.GetPosition();
    static_cast<void>(SafeSub(bytes_read, pos_start));
    return bytes_read;
}

}  // namespace helpers

// clang-format off
SharedMemTimeBaseReader::SharedMemTimeBaseReader(std::string_view name) noexcept
    : SharedMemTimeBaseReader(name, kSharedMemPageSize) {
}

// coverity[exn_spec_violation] Size of std::string_view will never exceed std::string.max_size(), so no length error will be thrown
SharedMemTimeBaseReader::SharedMemTimeBaseReader(std::string_view name, std::size_t max_size) noexcept
    : ITimeBaseAccessor{}, ITimeBaseReader{}, time_base_name_(name), max_size_(max_size) {
    shm_name_ = time_base_name_.front() != '/' ? std::string("/") + time_base_name_ : time_base_name_;
}
// clang-format on

SharedMemTimeBaseReader::~SharedMemTimeBaseReader() /* noexcept */ {
    Close();
}

void SharedMemTimeBaseReader::Open() /* noexcept */ {
    if (state_ == State::Open) {
        // allow Open() to be called repeatedly with (almost) no runtime penalty
        return;
    }

    auto fileDescriptor =
        OsShmOpen(shm_name_.c_str(), O_RDWR, static_cast<mode_t>(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    if (fileDescriptor < 0) {
        std::stringstream ss;
        ss << "SharedMemTimeBaseReader::Open() - "
           << "Unable to open shared memory file"
           << " for " << shm_name_.c_str() << " with error " << errno;
        logFatalAndAbort(ss.str().c_str());
    }

    // Open with write access to be able to create the TsyncReadWriteLock instance in
    // shared memory
    void* v_addr = OsMmap(nullptr, max_size_, PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    // clang-format off
    if (v_addr == MAP_FAILED) {
        // clang-format on
        OsClose(fileDescriptor);
        std::stringstream ss;
        ss << "SharedMemTimeBaseReader::Open() - OsMmap failed"
           << " for " << shm_name_.c_str() << " with error " << errno;
        logFatalAndAbort(ss.str().c_str());
    }
    addr_ = static_cast<char*>(v_addr);

    rw_lock_.Open(static_cast<pthread_rwlock_t*>(v_addr), TsyncReadWriteLock::LockMode::Read, false);
    // Now map shared mem again with readonly access
    v_addr = OsMmap(nullptr, max_size_, PROT_READ, MAP_SHARED, fileDescriptor, 0);
    OsClose(fileDescriptor);
    // clang-format off
    if (v_addr == MAP_FAILED) {
        // clang-format on
        std::stringstream ss;
        ss << "SharedMemTimeBaseReader::Open() - OsMmap failed"
           << " for " << shm_name_.c_str() << " with error " << errno;
        logFatalAndAbort(ss.str().c_str());
    }
    addr_read_only_ = static_cast<char*>(v_addr);

    // not testable, currently the alignment matches
    // if (current_read_offset_ % alignof(std::max_align_t) != 0) {
    //     AlignedSkip<SharedMemTimeBaseReader, std::max_align_t>(*this);
    // }

    state_ = State::Open;

    if (!(SetPosition(sizeof(TsyncReadWriteLock) + kSharedMemTimebaseDataOffset))) {
        logFatalAndAbort("SharedMemTimeBaseReader::Open() failed. MaxSize is too small.");
    }
}

ITimeBaseAccessor& SharedMemTimeBaseReader::GetAccessor() /* noexcept */ {
    return *this;
}

bool SharedMemTimeBaseReader::Read(std::uint8_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::uint16_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::uint32_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::uint64_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::int8_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::int16_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::int32_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(std::int64_t& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(float& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(double& data) /* noexcept */ {
    return (ReadField(data) > 0U);
}

bool SharedMemTimeBaseReader::Read(TimestampWithStatus& data) /* noexcept */ {
    return (ReadField(data) > 0u);
}

bool SharedMemTimeBaseReader::Read(VirtualLocalTime& data) /* noexcept */ {
    return (ReadField(data) > 0u);
}

bool SharedMemTimeBaseReader::Read(UserDataView& data) /* noexcept */ {
    // We're always reading 4 bytes of UserData
    constexpr std::size_t kUserMaxDataSize{4u};
    const char* p;
    bool res{Read(&p, kUserMaxDataSize)};

    if (res) {
        // clang-format off
        // RULECHECKER_comment(1, 1, check_reinterpret_cast, "This cast is well defined according to the C++ type aliasing rules.", true)
        UserDataView::pointer pb = reinterpret_cast<UserDataView::pointer>(p);
        // clang-format on
        // the actual user data length is stored in the first element.
        std::size_t sz = static_cast<unsigned char>(pb[0]);
        if (sz == 0u) {
            data = UserDataView{};
        } else if (sz > kUserMaxDataSize - 1u) {
            res = false;
        } else {
            data = UserDataView(&pb[1], sz);
        }
    }
    return res;
}

bool SharedMemTimeBaseReader::Read(SynchronizationStatus& data) /* noexcept */ {
    return (ReadField(data) > 0u);
}

// Note: this only works for types that have an alignment of 1 since we've switched to aligned reads/writes
bool SharedMemTimeBaseReader::Read(const char** p, std::size_t num_bytes) /* noexcept */ {
    if (state_ == State::Open && IsInBounds<std::size_t, std::size_t>(current_read_offset_, num_bytes, max_size_) &&
        p != nullptr) {
        *p = addr_read_only_ + current_read_offset_;
        return SetPosition(current_read_offset_ + num_bytes);
    }

    return false;
}

bool SharedMemTimeBaseReader::Read(TsyncTimeDomainConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_read_offset_;
        res = (SetPosition(kSharedMemTimebaseDomainConfigOffset) && (ReadField(data.eth_time_domain) != 0U) &&
               Read(data.consumer_config) && Read(data.provider_config) && (ReadField(data.sync_loss_timeout) != 0U) &&
               (ReadField(data.debounce_time) != 0U) && Read(data.domain_id));
        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseReader::Read(TsyncConsumerConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_read_offset_;
        res = (SetPosition(kSharedMemTimebaseConsumerConfigOffset) && Read(data.name) &&
               ReadField(data.time_slave_config) != 0u);

        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseReader::Read(TsyncProviderConfig& data) /* noexcept */ {
    bool res = false;

    if (state_ == State::Open) {
        std::size_t pos = current_read_offset_;

        res = (SetPosition(kSharedMemTimebaseProviderConfigOffset) && Read(data.name) &&
               (ReadField(data.time_master_config) != 0U) && (ReadField(data.time_sync_correction_config) != 0U));

        static_cast<void>(SetPosition(pos));
    }

    return res;
}

bool SharedMemTimeBaseReader::Read(std::string& data) /* noexcept */ {
    return (helpers::ReadField(*this, data) > 0);
}

bool SharedMemTimeBaseReader::Read(std::string_view& data) /* noexcept */ {
    return (helpers::ReadField(*this, data) != 0);
}

bool SharedMemTimeBaseReader::Skip(std::size_t num_bytes) /* noexcept */ {
    if (state_ == State::Open) {
        auto pos{current_read_offset_.load()};
        if (SafeAdd<std::size_t, std::size_t>(pos, num_bytes, max_size_)) {
            current_read_offset_ = pos;
            return true;
        }
    }

    return false;
}

bool SharedMemTimeBaseReader::SetPosition(std::size_t pos) /* noexcept */ {
    if (state_ == State::Open && pos < max_size_ && pos >= sizeof(TsyncReadWriteLock)) {
        current_read_offset_ = pos;
        return true;
    }

    return false;
}

std::size_t SharedMemTimeBaseReader::GetPosition() const /* noexcept */ {
    return current_read_offset_;
}

std::size_t SharedMemTimeBaseReader::GetMaxSize() const /* noexcept */ {
    return max_size_;
}

void SharedMemTimeBaseReader::lock() /* noexcept */ {
    if (state_ == State::Open) {
        rw_lock_.lock();
        if (!(SetPosition(sizeof(TsyncReadWriteLock) + kSharedMemTimebaseDataOffset))) {
            logFatalAndAbort("SharedMemTimeBaseReader::lock(): MaxSize is too small");
        }
    } else {
        logFatalAndAbort("lock called on closed reader");
    }
}

void SharedMemTimeBaseReader::unlock() /* noexcept */
{
    if (state_ == State::Open) {
        rw_lock_.unlock();
    } else {
        logFatalAndAbort("unlock called on closed reader");
    }
}

void SharedMemTimeBaseReader::Close() /* noexcept */ {
    if (state_ == ITimeBaseAccessor::State::Open) {
        state_ = ITimeBaseAccessor::State::Closed;

        if (addr_) {
            if (OsMunmap(addr_, max_size_) < 0) {
                std::cerr << "SharedMemTimeBaseReader::Close - munmap failed with " << errno << "\n";
            } else {
                addr_ = nullptr;
            }
        }

        if (addr_read_only_) {
            if (OsMunmap(addr_read_only_, max_size_) < 0) {
                std::cerr << "SharedMemTimeBaseReader::Close - munmap failed with " << errno << "\n";
            } else {
                addr_read_only_ = nullptr;
            }
        }
    }
}

std::string_view SharedMemTimeBaseReader::GetName() const /* noexcept */ {
    return time_base_name_;
}

ITimeBaseAccessor::State SharedMemTimeBaseReader::GetState() const /* noexcept */ {
    return state_;
}

}  // namespace time
}  // namespace score
