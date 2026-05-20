/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#include "score/time_daemon/src/application/svt/svt_handler.h"
#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/time_daemon/src/control_flow_divider/ptp/factory.h"
#include "score/time_daemon/src/ipc/svt/publisher/factory.h"
#include "score/time_daemon/src/msg_broker/subscription.h"
#include "score/time_daemon/src/msg_broker/topic.h"
#include "score/time_daemon/src/ptp_machine/shm/factory.h"
#include "score/time_daemon/src/verification_machine/svt/factory.h"
#include "score/concurrency/interruptible_wait.h"
#include "score/mw/log/logging.h"

#include <chrono>
#include <future>

namespace score
{
namespace td
{

SvtHandler::SvtHandler() noexcept
    : job_runner_{nullptr},
      msg_broker_{nullptr},
      gptp_machine_{nullptr},
      verification_machine_{nullptr},
      ipc_publisher_{nullptr},
      ctrl_flow_divider_{nullptr},
      handler_status_{TimebaseHandler::Status::kIdle}
{
    msg_broker_ = std::make_shared<MessageBroker<PtpTimeInfo>>();
    gptp_machine_ = CreateGPTPShmMachine("ptp_worker");
    verification_machine_ = CreateSvtVerificationMachine("time_verification_worker");
    ipc_publisher_ = CreateSvtPublisher("svt_ipc_publisher");
    ctrl_flow_divider_ = CreatePtpControlFlowDivider("ptp_control_flow_divider", std::chrono::milliseconds{250});

    std::vector<Job> jobs = {
        {[this] {
             return gptp_machine_->Init();
         },
         gptp_machine_->GetName(),
         std::chrono::seconds(20)},
        {[this] {
             return verification_machine_->Init();
         },
         verification_machine_->GetName(),
         std::chrono::seconds(20)},
        {[this] {
             return ipc_publisher_->Init();
         },
         ipc_publisher_->GetName(),
         std::chrono::seconds(20)},
        {[this] {
             return ctrl_flow_divider_->Init();
         },
         ctrl_flow_divider_->GetName(),
         std::chrono::seconds(20)},
    };
    job_runner_ = std::make_unique<JobRunner>(std::move(jobs), "svt_init");

    score::mw::log::LogInfo(kTimeBaseHandlerSvt) << "Handler created!";
}

void SvtHandler::Initialize() noexcept
{
    const auto input_ptp_data_topic = Topic("in_ptp_data");
    const auto raw_ptp_data_topic = Topic("raw_ptp_data");
    const auto validated_ptp_data_topic = Topic("validated_ptp_data");

    msg_broker_->AddSubscriber(input_ptp_data_topic, ctrl_flow_divider_);
    msg_broker_->AddSubscriber(raw_ptp_data_topic, verification_machine_);
    msg_broker_->AddSubscriber(validated_ptp_data_topic, ipc_publisher_);

    msg_broker_->AddProducer(input_ptp_data_topic, gptp_machine_);
    msg_broker_->AddProducer(raw_ptp_data_topic, ctrl_flow_divider_);
    msg_broker_->AddProducer(validated_ptp_data_topic, verification_machine_);

    score::mw::log::LogInfo(kTimeBaseHandlerSvt) << "Msg broker initialized!";
}

void SvtHandler::RunOnce(const score::cpp::stop_token& token) noexcept
{
    switch (handler_status_)
    {
        case TimebaseHandler::Status::kIdle:
        {
            job_runner_->Start(token);
            handler_status_ = TimebaseHandler::Status::kInitialize;
            score::mw::log::LogInfo(kTimeBaseHandlerSvt) << "Initialization started";
            break;
        }
        case TimebaseHandler::Status::kInitialize:
        {
            auto status = job_runner_->GetResult();
            switch (status)
            {
                case JobRunner::Result::kSucceed:
                {
                    score::mw::log::LogInfo(kTimeBaseHandlerSvt)
                        << "Initialization done, starting proactive machines!";
                    job_runner_.reset();
                    // start flow divider before ptp, as ptp might already try to publish some data
                    ctrl_flow_divider_->Start();
                    gptp_machine_->Start();
                    handler_status_ = TimebaseHandler::Status::kWorking;
                    score::mw::log::LogInfo(kTimeBaseHandlerSvt) << "Handler Is working";
                    break;
                }
                case JobRunner::Result::kFailed:
                {
                    handler_status_ = TimebaseHandler::Status::kFailed;
                    score::mw::log::LogError(kTimeBaseHandlerSvt) << "Initialization failed, handler not ready!!";
                    break;
                }
                case JobRunner::Result::kIdle:
                case JobRunner::Result::kInProgress:
                default:
                    break;
            }
            break;
        }
        case TimebaseHandler::Status::kWorking:
        case TimebaseHandler::Status::kFailed:
        default:
            break;
    }
}

void SvtHandler::Stop() noexcept
{
    if (handler_status_ == TimebaseHandler::Status::kWorking)
    {
        gptp_machine_->Stop();
        ctrl_flow_divider_->Stop();
        score::mw::log::LogInfo(kTimeBaseHandlerSvt) << "Stopping proactive machines!";
    }
}

}  // namespace td
}  // namespace score
