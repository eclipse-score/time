/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_TIMESLAVE_CODE_GPTP_GPTP_ENGINE_H
#define SCORE_TIMESLAVE_CODE_GPTP_GPTP_ENGINE_H

#include "score/TimeDaemon/code/common/data_types/ptp_time_info.h"
#include "score/TimeSlave/code/gptp/details/frame_codec.h"
#include "score/TimeSlave/code/gptp/details/i_network_identity.h"
#include "score/TimeSlave/code/gptp/details/i_raw_socket.h"
#include "score/TimeSlave/code/gptp/details/message_parser.h"
#include "score/TimeSlave/code/gptp/details/pdelay_measurer.h"
#include "score/TimeSlave/code/gptp/details/ptp_types.h"
#include "score/TimeSlave/code/gptp/details/sync_state_machine.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <pthread.h>
#include <string>

namespace score
{
namespace ts
{
namespace details
{

/// Configuration for GptpEngine.
struct GptpEngineOptions
{
    std::string  iface_name = "eth0";                             ///< Network interface for gPTP
    int          pdelay_interval_ms  = 1000;                      ///< Period between Pdelay_Req transmissions (ms)
    int          pdelay_warmup_ms    = 2000;                      ///< Delay before first Pdelay_Req (ms)
    int          sync_timeout_ms     = 3300;                      ///< Declare timeout after this many ms without Sync
    std::int64_t jump_future_threshold_ns = 500'000'000LL;        ///< 500 ms
};

/**
 * @brief gPTP engine for the TimeSlave process.
 *
 * Runs two POSIX threads: RxThread (receive/parse PTP frames) and
 * PdelayThread (periodic Pdelay_Req transmission).
 *
 * ReadPTPSnapshot() is thread-safe once Initialize() returns true.
 */
class GptpEngine final
{
  public:
    explicit GptpEngine(
        GptpEngineOptions                                        opts,
        std::unique_ptr<score::td::PtpTimeInfo::ReferenceClock>  local_clock) noexcept;

    /// Constructor for testing: inject fake socket and identity.
    GptpEngine(
        GptpEngineOptions                                        opts,
        std::unique_ptr<score::td::PtpTimeInfo::ReferenceClock>  local_clock,
        std::unique_ptr<IRawSocket>                              socket,
        std::unique_ptr<INetworkIdentity>                        identity) noexcept;

    ~GptpEngine() noexcept;

    GptpEngine(const GptpEngine&)            = delete;
    GptpEngine& operator=(const GptpEngine&) = delete;
    GptpEngine(GptpEngine&&)                 = delete;
    GptpEngine& operator=(GptpEngine&&)      = delete;

    /// Open the raw socket, enable HW timestamping, resolve the ClockIdentity,
    /// and start the Rx and Pdelay background threads.
    /// @return true on success.
    bool Initialize();

    /// Stop background threads and close the socket.
    /// @return true (always succeeds).
    bool Deinitialize();

    /// Copy the latest measurement snapshot into @p info.
    /// Non-blocking; returns false only if the engine is not initialized.
    bool ReadPTPSnapshot(score::td::PtpTimeInfo& info);

  private:
    static void* RxThreadEntry(void* arg) noexcept;
    static void* PdelayThreadEntry(void* arg) noexcept;
    void RxLoop() noexcept;
    void PdelayLoop() noexcept;

    void HandlePacket(const std::uint8_t* frame, int len,
                      const ::timespec& hwts) noexcept;
    void UpdateSnapshot(const SyncResult& sync,
                        const PDelayResult& pdelay) noexcept;

    GptpEngineOptions opts_;

    std::unique_ptr<score::td::PtpTimeInfo::ReferenceClock> local_clock_;
    std::unique_ptr<IRawSocket>                  socket_;
    std::unique_ptr<INetworkIdentity>            identity_;
    FrameCodec                                   codec_;
    GptpMessageParser                            parser_;
    SyncStateMachine                             sync_sm_;
    std::unique_ptr<PeerDelayMeasurer>           pdelay_;

    mutable std::mutex snapshot_mutex_;
    score::td::PtpTimeInfo        snapshot_{};

    std::atomic<bool> running_{false};
    pthread_t         rx_thread_{};
    pthread_t         pdelay_thread_{};
    bool              rx_started_{false};
    bool              pdelay_started_{false};
};

}  // namespace details
}  // namespace ts
}  // namespace score

#endif  // SCORE_TIMESLAVE_CODE_GPTP_GPTP_ENGINE_H
