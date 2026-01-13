/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "TsyncWorker.h"

#include <fcntl.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include "score/time/common/Abort.h"

#include "score/time/utility/ITimeBaseReader.h"
#include "score/time/utility/ITimeBaseWriter.h"
#include "score/time/utility/SysCalls.h"
#include "score/time/utility/TsyncIdMappingsHandler.h"
#include "score/time/utility/TsyncSharedUtils.h"

#include "ConfigLoader.h"
#include "HouseKeeping.h"
#include "TimeBaseConfiguration.h"

using score::time::common::logFatalAndAbort;
using SteadyClock = std::chrono::steady_clock;

namespace score {
namespace time {
namespace daemon {

TsyncWorker::TsyncWorker() noexcept {
}

TsyncWorker::~TsyncWorker() noexcept {
    ShutDown();
}

// coverity[exn_spec_violation] There will never be enough transmission semaphores for a length_error to be thrown
void TsyncWorker::Init() noexcept {
    if (!is_initialized_) {
        InitDaemon();
        HouseKeeping hk;
        hk.InitSignals();
        InitIdMappings();
        InitTimeBaseAccessors();

        is_initialized_ = true;
    } else {
        // TODO: log stuff
        std::cerr << "tsyncd: tsyncd: TsyncWorker::Init - TsyncWorker already initialized\n";
    }
    ::std::cout << "tsyncd: TsyncWorker::Init - done.\n";
}

void TsyncWorker::Run() noexcept {
    std::cout << "tsyncd: TsyncWorker::Run - enter\n";
    if (!is_initialized_) {
        // TODO: log stuff
        logFatalAndAbort("tsyncd: TsyncWorker::Run - TsyncWorker::Init must be called before TsyncWorker::Run");
    }
    std::cout << "tsyncd: TsyncWorker::Run - entering loop\n";
    while (HouseKeeping::exit_flag_ == 0) {
        CheckTimeouts();
        std::this_thread::sleep_until(SteadyClock::now() + std::chrono::seconds(1));
    }

    std::cout << "tsyncd: TsyncWorker::Run - received exit signal\n";
}

void TsyncWorker::CheckTimeouts() {
    auto& cfg = TimeBaseConfiguration::GetInstance();
    std::size_t i = 0u;

    for (auto it = cfg.begin(); it != cfg.end() && i < time_base_writers_.size(); ++it, ++i) {
        // check if we have a valid consumer config
        // timeouts are only relevant for consumers
        if (!it->second.timebase_config.consumer_config.time_slave_config.is_valid) {
            continue;
        }
        // check if we were ever synchronized and if the timeout state has already been set.
        // We may only switch to timeout state if we were synchronized at some point.
        // Resetting the timeout state will be done by ptpd, so if the flag is already set,
        // we do nothing
        score::time::SynchronizationStatus timeBaseStatus = GetTimestampStatus(time_base_readers_[i]);
        if (timeBaseStatus != SynchronizationStatus::kSynchronized) {
            continue;
        }
        auto ts_update_time{GetTimestampLastUpdateTime(time_base_readers_[i])};
        auto ts_current_time{GetTimestampCurrentVlt()};
        if (ts_current_time < ts_update_time) {
            continue;
        }

        auto vlt_diff = ts_current_time - ts_update_time;
        if (it->second.timebase_config.sync_loss_timeout < vlt_diff) {
            // coverity[autosar_cpp14_m0_1_3_violation:FALSE] #54006: Do not need to use the variables
            // defined by lock_guard.
            std::lock_guard<ITimeBaseWriter> lock(*time_base_writers_[i]);
            if (!time_base_writers_[i]->Write(SynchronizationStatus::kTimeOut)) {
                std::stringstream ss;
                ss << "TsyncWorker::CheckTimeouts() : Writing updated timeout status failed for "
                   << time_base_writers_[i]->GetAccessor().GetName();
                std::cerr << ss.str().c_str() << std::endl;
            } else {
            }
        }
    }
}

std::chrono::nanoseconds TsyncWorker::GetTimestampLastUpdateTime(
    TimeBaseReaderFactory::PointerType& tb_reader) {
    std::chrono::nanoseconds res{0};
    // coverity[autosar_cpp14_m0_1_3_violation:FALSE] #54006: Do not need to use the variables defined by lock_guard.
    std::lock_guard<ITimeBaseReader> lock{*tb_reader};
    if (AlignedSkip<ITimeBaseReader, TimestampWithStatus>(*tb_reader)) {
        if (tb_reader->Read(res)) {
        } else {
            std::cerr << "TsyncWorker::GetTimestampLastUpdateTime() : Read virtual local time failed\n";
        }
    } else {
        logFatalAndAbort("TsyncWorker::GetTimestampLastUpdateTime() : Skip failed.");
    }

    return res;
}

std::chrono::nanoseconds TsyncWorker::GetTimestampCurrentVlt() {
    std::chrono::nanoseconds res{0};

    auto vlt = TsyncSharedUtils::GetCurrentVirtualLocalTime();
    if (vlt) {
        res = *vlt;
    }

    return res;
}

score::time::SynchronizationStatus TsyncWorker::GetTimestampStatus(
    TimeBaseReaderFactory::PointerType& tb_reader) {
    SynchronizationStatus status{SynchronizationStatus::kNotSynchronizedUntilStartup};
    // coverity[autosar_cpp14_m0_1_3_violation:FALSE] #54006: Do not need to use the variables defined by lock_guard.
    std::lock_guard<ITimeBaseReader> lock{*tb_reader};
    bool res = tb_reader->Read(status);
    if (!res) {
        std::cerr << "TsyncWorker::GetTimestampStatus(" << tb_reader->GetAccessor().GetName()
                  << "): failed to read status." << std::endl;
        status = SynchronizationStatus::kNotSynchronizedUntilStartup;
    }

    return status;
}

void TsyncWorker::ShutDown() noexcept {
    if (is_initialized_) {
        CloseTimeBaseAccessors();
        transmission_semaphores_.clear();
        is_initialized_ = false;
    }
}

void TsyncWorker::InitDaemon() {
    const mode_t mask = 007u;
    (void)OsUmask(mask);
}

void TsyncWorker::InitTimeBaseAccessors() {
    auto& cfg = TimeBaseConfiguration::GetInstance();

    std::size_t i = 0u;
    for (auto it = cfg.begin(); it != cfg.end() && i < time_base_writers_.size(); ++it, ++i) {
        time_base_writers_[i] = TimeBaseWriterFactory::Create(it->first, true);
        time_base_readers_[i] = TimeBaseReaderFactory::Create(it->first);
        time_base_writers_[i]->GetAccessor().Open();
        time_base_readers_[i]->GetAccessor().Open();

        // create transmission semaphore for timebase providers
        if (it->second.timebase_config.provider_config.time_master_config.is_valid) {
            mode_t temp_umask = OsUmask(0111u);
            transmission_semaphores_.push_back(
                TsyncSharedUtils::CreateTransmissionSemaphore(it->second.timebase_config.domain_id, true));
            (void)OsUmask(temp_umask);
        }

        {
            // coverity[autosar_cpp14_m0_1_3_violation:FALSE] #54006: Do not need to use the variables defined by
            // lock_guard.
            std::lock_guard<ITimeBaseWriter> lock(*time_base_writers_[i]);
            time_base_writers_[i]->WriteDefaults();
            time_base_writers_[i]->Write(it->second.timebase_config);
        }
    }
}

void TsyncWorker::InitIdMappings() {
    id_mappings_handler_ = std::make_unique<TsyncIdMappingsHandler>();
    auto& cfg = TimeBaseConfiguration::GetInstance();

    for (auto& it : cfg) {
        auto res = id_mappings_handler_->AddDomainMapping(it.second.timebase_config.domain_id,
                                                          it.second.timebase_name.c_str());
        if (res) {
            if (it.second.timebase_config.consumer_config.time_slave_config.is_valid) {
                // TODO: We currently only allow one consumer per time domain
                res = id_mappings_handler_->AddConsumerToDomain(it.second.timebase_config.domain_id,
                                                                it.second.timebase_config.consumer_config.name);
                if (!res) {
                    logFatalAndAbort("TsyncWorker::InitIdMappings(): Error adding consumer to time domain mapping.");
                }
            }
            if (it.second.timebase_config.provider_config.time_master_config.is_valid) {
                res = id_mappings_handler_->AddProviderToDomain(it.second.timebase_config.domain_id,
                                                                it.second.timebase_config.provider_config.name);
                if (!res) {
                    logFatalAndAbort("TsyncWorker::InitIdMappings(): Error adding provider to time domain mapping.");
                }
            }
        } else {
            logFatalAndAbort("TsyncWorker::InitIdMappings(): Error adding domain mapping.");
        }
    }

    id_mappings_handler_->CommitMappingsToSharedMemory();
}

void TsyncWorker::CloseTimeBaseAccessors() {
    id_mappings_handler_.reset();

    for (auto& p : time_base_writers_) {
        if (!p) {
            break;
        }

        p.reset();
    }

    for (auto& p : time_base_readers_) {
        if (!p) {
            break;
        }

        p.reset();
    }
}

}  // namespace daemon
}  // namespace time
}  // namespace score
