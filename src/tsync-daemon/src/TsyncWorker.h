/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_DAEMON_TSYNCWORKER_H_
#define SCORE_TIME_DAEMON_TSYNCWORKER_H_

#include <array>
#include <chrono>
#include <memory>
#include <vector>

#include "score/time/synchronized_time_base_status.h"
#include "score/time/utility/TimeBaseReaderFactory.h"
#include "score/time/utility/TimeBaseWriterFactory.h"
#include "score/time/utility/TsyncNamedSemaphore.h"

namespace score {
namespace time {

class TsyncIdMappingsHandler;

namespace daemon {

// note: this class is currently not implemented in a thread-safe fashion
class TsyncWorker final {
public:
    TsyncWorker() noexcept;
    ~TsyncWorker() noexcept;
    TsyncWorker(const TsyncWorker&) = delete;
    TsyncWorker& operator=(const TsyncWorker&) = delete;

    void Init() noexcept;
    void Run() noexcept;
    void ShutDown() noexcept;

private:
    void InitDaemon();
    void InitIdMappings();
    void InitTimeBaseAccessors();
    void CloseTimeBaseAccessors();
    void CheckTimeouts();
    std::chrono::nanoseconds GetTimestampLastUpdateTime(TimeBaseReaderFactory::PointerType& tb_reader);
    std::chrono::nanoseconds GetTimestampCurrentVlt();
    SynchronizationStatus GetTimestampStatus(TimeBaseReaderFactory::PointerType& tb_reader);

    using TimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration>;

private:
    std::unique_ptr<TsyncIdMappingsHandler> id_mappings_handler_{};
    std::array<TimeBaseWriterFactory::PointerType, 8> time_base_writers_{};
    std::array<TimeBaseReaderFactory::PointerType, 8> time_base_readers_{};
    std::vector<TsyncNamedSemaphore> transmission_semaphores_{};
    bool is_initialized_ = false;
};

}  // namespace daemon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_DAEMON_TSYNCWORKER_H_
