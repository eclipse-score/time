/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TsyncNamedSemaphore.h"

#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */

#include <cstring>
#include <sstream>

#include "score/time/common/Abort.h"

#include "SysCalls.h"

using score::time::common::logFatalAndAbort;

namespace score {
namespace time {

TsyncNamedSemaphore::TsyncNamedSemaphore(const std::string& name, OpenMode openMode, bool is_owner)
    : is_owner_{is_owner} {
    name_ = (name.front() == '/') ? name : std::string("/").append(name);
    int oFlags = is_owner ? (O_CREAT | O_EXCL) : 0;
    mode_t mode = static_cast<mode_t>(S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP);
    sem_ = OsSemOpen(name_.c_str(), oFlags, mode, (openMode == OpenMode::Signaled ? 1u : 0u));
    // clang-format off
    // RULECHECKER_comment(1, 1, check_c_style_cast, "Use of C-style cast forced by underlying POSIX API semantics.", true)
    if (sem_ == SEM_FAILED) {
        // clang-format on
        std::stringstream ss;
        ss << "TsyncNamedSemaphore::TsyncNamedSemaphore - failed to create semaphore " << name << " with error "
           << std::strerror(errno);

        logFatalAndAbort(ss.str().c_str());
    }
}

TsyncNamedSemaphore::~TsyncNamedSemaphore() {
    Destroy();
}

TsyncNamedSemaphore::TsyncNamedSemaphore(TsyncNamedSemaphore&& rhs) {
    Move(std::forward<TsyncNamedSemaphore>(rhs));
}

TsyncNamedSemaphore& TsyncNamedSemaphore::operator=(TsyncNamedSemaphore&& rhs) {
    Destroy();
    Move(std::forward<TsyncNamedSemaphore>(rhs));
    return *this;
}

void TsyncNamedSemaphore::lock() noexcept {
    while (OsSemWait(sem_) != 0) {
        if (errno != EINTR) {
            std::stringstream ss;
            ss << "TsyncNamedSemaphore::lock() - sem_wait failed for semaphore " << name_ << " with error "
               << std::strerror(errno);

            logFatalAndAbort(ss.str().c_str());
        }
    }
}

void TsyncNamedSemaphore::unlock() noexcept {
    if (OsSemPost(sem_) != 0) {
        std::stringstream ss;
        ss << "TsyncNamedSemaphore::unlock() - sem_post failed for semaphore " << name_ << " with error "
           << std::strerror(errno);

        logFatalAndAbort(ss.str().c_str());
    }
}

bool TsyncNamedSemaphore::try_lock() noexcept {
    return (OsSemTryWait(sem_) == 0);
}

int TsyncNamedSemaphore::get_value() const noexcept {
    int val;

    if (OsSemGetValue(sem_, &val) != 0) {
        // since we currently do not care for blocked processes,
        // only for pending signals, indicating errors with -1
        // is acceptable
        val = -1;
    }

    return val;
}

void TsyncNamedSemaphore::Move(TsyncNamedSemaphore&& rhs) {
    sem_ = rhs.sem_;
    is_owner_ = rhs.is_owner_;
    name_ = rhs.name_;

    // clang-format off
    // RULECHECKER_comment(1, 1, check_c_style_cast, "Use of C-style cast forced by underlying POSIX API semantics.", true)
    rhs.sem_ = SEM_FAILED;
    // clang-format on
}

void TsyncNamedSemaphore::Destroy() {
    // clang-format off
    // RULECHECKER_comment(1, 1, check_c_style_cast, "Use of C-style cast forced by underlying POSIX API semantics.", true)
    if (sem_ != SEM_FAILED) {
        // clang-format on
        if (OsSemClose(sem_) != 0) {
            std::stringstream ss;
            ss << "TsyncNamedSemaphore::Destroy() - sem_close failed for semaphore " << name_ << " with error "
               << std::strerror(errno);

            logFatalAndAbort(ss.str().c_str());
        }

        // clang-format off
        // RULECHECKER_comment(1, 1, check_c_style_cast, "Use of C-style cast forced by underlying POSIX API semantics.", true)
        sem_ = SEM_FAILED;
        // clang-format on

        if (is_owner_) {
            if (OsSemUnlink(name_.c_str()) != 0) {
                std::stringstream ss;
                ss << "TsyncNamedSemaphore::Destroy() - sem_unlink failed for semaphore " << name_ << " with error "
                   << std::strerror(errno);

                logFatalAndAbort(ss.str().c_str());
            }
        }
    }
}

}  // namespace time
}  // namespace score
