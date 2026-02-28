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
#pragma once
#include "tsync_types.hpp"
#include "ntp_client.hpp"
#include "score_time/ipc/shared_state.hpp"
#include "score_time/ipc/shm_region.hpp"
#include <array>
#include <atomic>
#include <string>
#include <vector>

namespace tsyncd
{

    /**
     * @brief Configuration options for TimeSyncEngine
     *
     * Contains all configurable parameters for the tsyncd daemon including:
     * - gPTP network interface and PHC device
     * - Shared memory configuration
     * - NTP client settings for absolute time
     * - External time source configuration
     * - Timeout and threshold values
     */
    struct EngineOptions
    {
        std::string iface_name = kDefaultIfaceName;      ///< Network interface for gPTP (e.g., "eth0")
        std::string phc_device = kDefaultPhcDevice;      ///< PTP Hardware Clock device (e.g., "/dev/ptp0")
        std::string shm_name = "/score_time_vehicle_time"; ///< Shared memory object name
        std::size_t shm_size = 4096;                     ///< Shared memory size in bytes

        /**
         * @brief Absolute time operation mode
         */
        enum class AbsMode : std::uint8_t
        {
            kPublishOnly = 0,           ///< Only publish to shared memory, don't adjust system clock
            kDisciplineSystemClock = 1  ///< Discipline system clock (requires CAP_SYS_TIME)
        };
        AbsMode abs_mode = AbsMode::kPublishOnly;

        // NTP client configuration
        std::vector<std::string> ntp_servers = {"pool.ntp.org"}; ///< NTP server hostnames/IPs
        int ntp_port = 123;                          ///< NTP server port (default 123)
        int ntp_query_interval_ms = 1000;            ///< NTP query interval in milliseconds
        int ntp_request_timeout_ms = 250;            ///< NTP request timeout in milliseconds
        int ntp_samples_to_lock = 3;                 ///< Samples needed before considering NTP locked
        double ntp_offset_ewma_alpha = 0.2;          ///< EWMA smoothing factor for offset (0-1)
        double ntp_jitter_ewma_alpha = 0.2;          ///< EWMA smoothing factor for jitter (0-1)

        int abs_publish_interval_ms = 200;           ///< Absolute time publish interval to shared memory

        // External absolute time source configuration
        bool abs_external_enable = false;            ///< Enable external time source (GNSS, etc.)
        std::string abs_source_socket = "/run/score_time/abs_time_source.sock"; ///< Unix socket path
        int abs_source_timeout_ms = 5000;            ///< External source timeout in milliseconds

        int pdelay_req_interval_ms = 1000;           ///< gPTP peer delay request interval

        // Timeout thresholds (0 = disabled)
        int sync_timeout_ms = 0;                     ///< Sync message timeout (detect master loss)
        int pdelay_timeout_ms = 0;                   ///< Peer delay timeout (detect link down)

        // Time quality thresholds
        std::int64_t unstable_offset_threshold_ns = 10'000;   ///< Offset threshold for unstable state
        std::int64_t jump_future_threshold_ns = 600'000'000;  ///< Jump detection threshold (600ms)
    };

    /**
     * @brief Main time synchronization engine for tsyncd daemon
     *
     * Implements gPTP (IEEE 802.1AS) slave functionality and publishes synchronized
     * time to shared memory for IPC with client applications.
     *
     * **Architecture:**
     * - RxLoop: Receives and processes gPTP messages (Sync, Follow_Up, Pdelay_Resp, etc.)
     * - PdelayLoop: Periodically sends peer delay requests to measure link delay
     * - AbsLoop: Queries NTP/external sources and publishes absolute time
     *
     * **Thread Safety:**
     * - Three worker threads managed by Start()/Stop()
     * - Uses mutexes for internal state synchronization
     * - Publishes to shared memory using lock-free seqlock protocol
     *
     * **Lifecycle:**
     * 1. Construct with EngineOptions
     * 2. Call Start() to initialize resources and launch threads
     * 3. Threads run until Stop() is called
     * 4. Destructor ensures cleanup
     *
     * @note This class is move-only (implicitly via deleted copy constructors)
     * @warning Only one instance should run per network interface
     */
    class TimeSyncEngine final
    {
    public:
        explicit TimeSyncEngine(const EngineOptions &opt);
        ~TimeSyncEngine();

        /**
         * @brief Start the synchronization engine
         *
         * Initializes PHC, raw socket, shared memory, and launches worker threads.
         *
         * @return true if all initialization succeeded and threads started
         * @return false if any initialization failed (check logs for details)
         *
         * @note Acquires resources (file descriptors, shared memory, threads)
         * @note If Start() fails, Stop() is called automatically to clean up
         */
        bool Start();

        /**
         * @brief Stop the synchronization engine
         *
         * Signals worker threads to stop, waits for them to exit, and releases resources.
         * Safe to call multiple times or even if Start() failed.
         *
         * @note Blocks until all threads have exited (may take up to ~1 second)
         */
        void Stop();

        void RxLoop();      ///< Worker thread: receive and process gPTP messages
        void PdelayLoop();  ///< Worker thread: send periodic peer delay requests
        void AbsLoop();     ///< Worker thread: query and publish absolute time

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

        std::array<unsigned char, 2048> rx_buffer_{};
    };

}
