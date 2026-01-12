/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TSYNCREADWRITELOCK_H_
#define SCORE_TIME_UTILITY_TSYNCREADWRITELOCK_H_

#include <pthread.h>
#include <functional>

namespace score {
namespace time {

class TsyncReadWriteLock final {
public:
    enum class LockMode { Read, Write };

    TsyncReadWriteLock();
    TsyncReadWriteLock(const TsyncReadWriteLock&) = delete;
    TsyncReadWriteLock& operator=(const TsyncReadWriteLock&) = delete;
    TsyncReadWriteLock(TsyncReadWriteLock&&);
    TsyncReadWriteLock& operator=(TsyncReadWriteLock&&);
    ~TsyncReadWriteLock() noexcept;

    void Open(pthread_rwlock_t* rw_lock_addr, LockMode mode, bool is_owner) noexcept;
    void Close() noexcept;

    bool try_lock() noexcept;
    void lock() noexcept;
    void unlock() noexcept;

private:
    pthread_rwlock_t* rw_lock_{};
    std::function<int(pthread_rwlock_t*)> lock_function_{};
    std::function<int(pthread_rwlock_t*)> try_lock_function_{};
    bool is_owner_{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TSYNCREADWRITELOCK_H_
