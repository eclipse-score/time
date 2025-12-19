/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TsyncReadWriteLock.h"

#include <cstring>
#include <sstream>

#include "score/time/common/Abort.h"

#include "SysCalls.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

TsyncReadWriteLock::TsyncReadWriteLock() : rw_lock_(nullptr) {
}

TsyncReadWriteLock::~TsyncReadWriteLock() noexcept {
    Close();
}

void TsyncReadWriteLock::Open(pthread_rwlock_t* rw_lock_addr, LockMode mode, bool is_owner) noexcept {
    Close();
    is_owner_ = is_owner;
    rw_lock_ = rw_lock_addr;

    if (is_owner_) {
        pthread_rwlockattr_t attr;
        int res = OsRwLockInitAttr(&attr);
        if (res != 0) {
            std::stringstream ss;
            ss << "TsyncReadWriteLock::TsyncReadWriteLock() - pthreadrw_lock_attr_init failed. [" << std::strerror(res)
               << "]";
            logFatalAndAbort(ss.str().c_str());
        }
        res = OsRwLockAttrSetPShared(&attr, PTHREAD_PROCESS_SHARED);
        if (res != 0) {
            std::stringstream ss;
            ss << "TsyncReadWriteLock::TsyncReadWriteLock() - pthreadrw_lock_attr_setpshared failed. ["
               << std::strerror(res) << "]";
            logFatalAndAbort(ss.str().c_str());
        }
        res = OsRwLockInit(rw_lock_, &attr);
        if (res != 0) {
            std::stringstream ss;
            ss << "TsyncReadWriteLock::TsyncReadWriteLock() - pthreadrw_lock_init failed. [" << std::strerror(res)
               << "]";
            logFatalAndAbort(ss.str().c_str());
        }
        (void)OsRwLockDestroyAttr(&attr);
    } else {
    }
    if (mode == LockMode::Read) {
        try_lock_function_ = &OsRwLockTryReadLock;
        lock_function_ = &OsRwLockReadLock;
    } else {
        try_lock_function_ = &OsRwLockTryWriteLock;
        lock_function_ = &OsRwLockWriteLock;
    }
}

void TsyncReadWriteLock::Close() noexcept {
    if (rw_lock_ && is_owner_) {
        OsRwLockDestroyLock(rw_lock_);
        rw_lock_ = nullptr;
    }
}

TsyncReadWriteLock::TsyncReadWriteLock(TsyncReadWriteLock&& rhs) : rw_lock_{rhs.rw_lock_}, is_owner_{rhs.is_owner_} {
    rhs.rw_lock_ = nullptr;
}

TsyncReadWriteLock& TsyncReadWriteLock::operator=(TsyncReadWriteLock&& rhs) {
    rw_lock_ = rhs.rw_lock_;
    is_owner_ = rhs.is_owner_;

    rhs.rw_lock_ = nullptr;
    return *this;
}

bool TsyncReadWriteLock::try_lock() noexcept {
    return (try_lock_function_(rw_lock_) == 0);
}

void TsyncReadWriteLock::lock() noexcept {
    int res = lock_function_(rw_lock_);

    if (res != 0) {
        logFatalAndAbort("TsyncReadWriteLock::lock() failed.");
    }
}

void TsyncReadWriteLock::unlock() noexcept {
    int res = OsRwLockUnlock(rw_lock_);

    if (res != 0) {
        logFatalAndAbort("TsyncReadWriteLock::unlock() failed.");
    }
}

}  // namespace time
}  // namespace score
