/********************************************************************************************************
# Copyright ©, 2026, ECARX. All rights reserved. This statement is the current and authoritative version.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: chenhao.gao chenhao.gao@ecarxgroup.com
*********************************************************************************************************/
#pragma once
#include "tsync_types.hpp"
#include "ntp_client.hpp"
#include "score_time/ipc/shared_state.hpp"
#include "score_time/ipc/shm_region.hpp"
#include <atomic>
#include <string>
#include <vector>

namespace tsyncd
{

    struct EngineOptions
    {
        std::string iface_name = kDefaultIfaceName;
        std::string phc_device = kDefaultPhcDevice;
        std::string shm_name = "/score_time_vehicle_time";
        std::size_t shm_size = 4096;

        enum class AbsMode : std::uint8_t
        {
            kPublishOnly = 0,
            kDisciplineSystemClock = 1
        };
        AbsMode abs_mode = AbsMode::kPublishOnly;

        std::vector<std::string> ntp_servers = {"pool.ntp.org"};
        int ntp_port = 123;
        int ntp_query_interval_ms = 1000;
        int ntp_request_timeout_ms = 250;
        int ntp_samples_to_lock = 3;
        double ntp_offset_ewma_alpha = 0.2;
        double ntp_jitter_ewma_alpha = 0.2;

        int abs_publish_interval_ms = 200;

        bool abs_external_enable = false;
        std::string abs_source_socket = "/run/score_time/abs_time_source.sock";
        int abs_source_timeout_ms = 5000;

        int pdelay_req_interval_ms = 1000;

        int sync_timeout_ms = 0;
        int pdelay_timeout_ms = 0;

        std::int64_t unstable_offset_threshold_ns = 10'000;
        std::int64_t jump_future_threshold_ns = 600'000'000;
    };

    class TimeSyncEngine final
    {
    public:
        explicit TimeSyncEngine(const EngineOptions &opt);
        ~TimeSyncEngine();

        bool Start();
        void Stop();

        void RxLoop();
        void PdelayLoop();
        void AbsLoop();

    private:
        bool InitPhc();
        bool InitRawSocket();
        bool InitHwTimestamping();
        bool InitClockIdentity();
        bool InitShm();
        bool InitAbsSourceSocket();

        void PublishAbsoluteFromNtp();
        void PublishAbsoluteFromExternal();
        score_time::AbsoluteAccuracyQualifier MapInaccuracyToQual(std::int64_t inacc_ns) const;

        void HandlePacket(const unsigned char *frame, int frame_len, const ::timespec &hwts);
        void SyncFupStateMachine(TsyncEvent ev, const PTPMessage &msg);
        int SendPdelayRequest();
        int SendPdelayRespAndFup(const PTPMessage &req);

        void ComputePeerDelay();

        void PortSynchronize(const tmv_t &sync_hw_ts,
                             const tmv_t &fup_msg_ts,
                             const tmv_t &sync_corr,
                             const tmv_t &fup_corr,
                             std::uint16_t seq_id);

        static tmv_t timespec_to_tmv(const ::timespec &ts);
        static tmv_t correction_to_tmv(std::int64_t corr);
        static tmv_t Timestamp_to_tmv(const Timestamp &ts);
        static Timestamp tmv_to_Timestamp(const tmv_t &x);
        static void normalize_timespec(::timespec &ts);

    private:
        EngineOptions opt_;
        std::atomic<bool> stop_{false};

        Context ctx_{};

        score_time::ipc::ShmRegion shm_;
        score_time::ipc::SharedState *shared_ = nullptr;

        ntp::Client ntp_client_{ntp::Client::Options{}};
        ntp::Estimator ntp_estimator_{ntp::Estimator::Options{}};
        std::int64_t next_ntp_query_mono_ns_ = 0;

        std::atomic<std::int64_t> last_sync_event_mono_ns_{0};
        std::atomic<std::int64_t> last_pdelay_event_mono_ns_{0};
        std::atomic<std::int64_t> last_pdelay_req_mono_ns_{0};
        std::atomic<bool> pdelay_waiting_resp_{false};
        std::atomic<std::uint32_t> pdelay_consecutive_loss_count_{0};
        std::atomic<bool> sync_timeout_logged_{false};
        std::atomic<bool> pdelay_timeout_logged_{false};

        int abs_sock_fd_ = -1;
        struct ExternalAbsSample
        {
            std::int64_t utc_ns{0};
            std::int64_t inaccuracy_ns{0};
            score_time::AbsoluteSecurityQualifier sec{score_time::AbsoluteSecurityQualifier::kNoTimeAvailable};
            std::int64_t mono_rx_ns{0};
            bool valid{false};
        } abs_ext_;

        pthread_t rx_th_{};
        pthread_t pdelay_th_{};
        pthread_t abs_th_{};
        bool rx_started_ = false;
        bool pdelay_started_ = false;
        bool abs_started_ = false;
    };

}
