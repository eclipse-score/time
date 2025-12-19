/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_UTILITY_TSYNCNAMEDSEMAPHORE_H_
#define SCORE_TIME_UTILITY_TSYNCNAMEDSEMAPHORE_H_

#include <string>
#include <semaphore.h>

namespace score {
namespace time {

class TsyncNamedSemaphore final {
public:
    enum class OpenMode { Signaled, Unsignaled };

    TsyncNamedSemaphore(const std::string& name, OpenMode openMode, bool is_owner = false);
    TsyncNamedSemaphore() = delete;
    TsyncNamedSemaphore(const TsyncNamedSemaphore&) = delete;
    TsyncNamedSemaphore& operator=(const TsyncNamedSemaphore&) = delete;
    TsyncNamedSemaphore(TsyncNamedSemaphore&&);
    TsyncNamedSemaphore& operator=(TsyncNamedSemaphore&&);
    ~TsyncNamedSemaphore();

    bool try_lock() noexcept;
    int get_value() const noexcept;
    // std::BasicLockable requirements
    void lock() noexcept;
    void unlock() noexcept;

private:
    void Destroy();
    void Move(TsyncNamedSemaphore&&);

    std::string name_{};
    // clang-format off
    // RULECHECKER_comment(1, 1, check_c_style_cast, "Use of C-style cast forced by underlying POSIX API semantics.", true)
    sem_t* sem_{SEM_FAILED};
    // clang-format on
    bool is_owner_{};
};

}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_UTILITY_TSYNCNAMEDSEMAPHORE_H_
