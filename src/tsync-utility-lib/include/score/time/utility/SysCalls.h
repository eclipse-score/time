/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_SYSCALLS_H_
#define SCORE_TIME_UTILITY_SYSCALLS_H_

#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace score {
namespace time {

int OsShmOpen(const char* name, int oflag, mode_t mode);
int OsShmUnlink(const char* name);
int OsFtruncate(int fd, off_t length);
void* OsMmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);
int OsMunmap(void* addr, size_t len);
int OsClose(int fd);
mode_t OsUmask(mode_t mode);
sem_t* OsSemOpen(const char* name, int oflag);
sem_t* OsSemOpen(const char* name, int oflag, mode_t mode, unsigned int value);
int OsSemClose(sem_t* sem);
int OsSemUnlink(const char* name);
int OsSemWait(sem_t* sem);
int OsSemPost(sem_t* sem);
int OsSemTryWait(sem_t* sem);
int OsSemGetValue(sem_t* sem, int* value);
int OsRwLockInitAttr(pthread_rwlockattr_t* attr);
int OsRwLockDestroyAttr(pthread_rwlockattr_t* attr);
int OsRwLockAttrSetPShared(pthread_rwlockattr_t* attr, int shared);
int OsRwLockInit(pthread_rwlock_t* rw_lock, const pthread_rwlockattr_t* attr);
int OsRwLockTryReadLock(pthread_rwlock_t* rw_lock);
int OsRwLockReadLock(pthread_rwlock_t* rw_lock);
int OsRwLockTryWriteLock(pthread_rwlock_t* rw_lock);
int OsRwLockWriteLock(pthread_rwlock_t* rw_lock);
int OsRwLockUnlock(pthread_rwlock_t* rw_lock);
int OsRwLockDestroyLock(pthread_rwlock_t* rw_lock);
int OsClockGetTime(clockid_t clk_id, timespec* tp);
int OsThreadCreate(pthread_t* thread, const pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);
int OsThreadJoin(pthread_t thread, void** retval);
mode_t OsUmask(mode_t mode);

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_SYSCALLS_H_
