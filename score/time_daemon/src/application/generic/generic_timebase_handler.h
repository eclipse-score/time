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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H

#include "score/mw/log/logging.h"
#include "score/time_daemon/src/application/job_runner/job_runner.h"
#include "score/time_daemon/src/application/timebase_handler.h"
#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"
#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/time_daemon/src/common/machines/base_machine.h"
#include "score/time_daemon/src/common/machines/proactive_machine.h"
#include "score/time_daemon/src/msg_broker/msg_broker.h"
#include "score/time_daemon/src/msg_broker/topic.h"

#include <score/assert.hpp>
#include <score/stop_token.hpp>

#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace score
{
namespace td
{

/// \brief Timebase handler whose machines share a MessageBroker.
///
/// A derived handler adds machines in CreateComponents(). Use Publish() and Subscribe() to connect them.
/// Do not add or connect machines after Initialize().
///
/// The JobRunner initializes the machines on a worker thread. Once all machines are ready, proactive machines
/// start in Add() order and stop in reverse order. Add machines that must run first before their data sources.
///
/// \tparam T Type of messages sent through the MessageBroker.
template <typename T>
class GenericTimebaseHandler : public TimebaseHandler
{
  public:
    /// \param name Name used in log messages and for the initialization thread.
    explicit GenericTimebaseHandler(std::string name);

    /// \brief Stops the machines and waits for initialization before freeing them.
    ~GenericTimebaseHandler() noexcept override;

    /// \brief Adds a machine. The JobRunner retries Init() until it succeeds or init_timeout expires.
    /// A timeout makes the handler fail.
    /// \return The machine passed to Add().
    template <typename M>
    std::shared_ptr<M> Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout);

    /// \brief Sets the producer's topic. Each producer can use only one topic.
    void Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic);

    /// \brief Registers the consumer for messages published on the topic.
    void Subscribe(std::weak_ptr<Consumer<T>> consumer, const Topic& topic);

    /// \brief Calls CreateComponents() and sets up initialization. Call once, before RunOnce().
    void Initialize() noexcept override;

    /// \brief Starts initialization on the first call. Starts proactive machines once all machines are ready.
    /// \param token Stop token for initialization and machine shutdown.
    void RunOnce(const score::cpp::stop_token& token) noexcept override;

    /// \brief Stops running proactive machines in reverse start order.
    void Stop() noexcept override;

  protected:
    /// \brief Adds and connects machines. Initialize() calls this once.
    virtual void CreateComponents() noexcept = 0;

  private:
    void PollInitialization(const score::cpp::stop_token& token) noexcept;
    void StartMachines() noexcept;
    void MoveToIdleState() noexcept;
    void MoveToInitializeState() noexcept;
    void MoveToWorkingState() noexcept;
    void MoveToFailedState() noexcept;

    std::shared_ptr<MessageBroker<T>> broker_;
    /// Keeps the machines alive. The MessageBroker holds only weak references.
    std::vector<std::shared_ptr<BaseMachine>> machines_;
    std::vector<std::shared_ptr<ProactiveMachine>> start_order_;  // in Add() order
    const std::string name_;
    std::vector<Job> init_jobs_;
    /// Tracks producers to reject duplicate Publish() calls.
    std::set<std::weak_ptr<Producer<T>>, std::owner_less<std::weak_ptr<Producer<T>>>> producers_;
    std::unique_ptr<JobRunner> init_job_runner_;
    TimebaseHandler::Status handler_status_;
    /// Set by Initialize(). The machines may still be initializing.
    bool is_configured_;
};

template <typename T>
GenericTimebaseHandler<T>::GenericTimebaseHandler(std::string name)
    : broker_{std::make_shared<MessageBroker<T>>()},
      machines_{},
      start_order_{},
      name_{std::move(name)},
      init_jobs_{},
      producers_{},
      init_job_runner_{nullptr},
      handler_status_{TimebaseHandler::Status::kIdle},
      is_configured_{false}
{
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": handler created";
}

template <typename T>
GenericTimebaseHandler<T>::~GenericTimebaseHandler() noexcept
{
    Stop();
    init_job_runner_.reset();
}

template <typename T>
template <typename M>
std::shared_ptr<M> GenericTimebaseHandler<T>::Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_configured_, "Machines must be added before Initialize()");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(machine != nullptr, "Machine must not be nullptr");

    auto init_fn = [machine]() {
        return machine->Init();
    };
    init_jobs_.push_back(Job{init_fn, machine->GetName(), init_timeout});
    machines_.push_back(machine);
    if constexpr (std::is_base_of_v<ProactiveMachine, M>)
    {
        start_order_.push_back(machine);
    }
    return machine;
}

template <typename T>
void GenericTimebaseHandler<T>::Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_configured_, "Publish() must be called before Initialize()");
    const auto is_new_producer = producers_.insert(producer).second;
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_new_producer, "A producer publishes on one topic only");
    broker_->AddProducer(topic, std::move(producer));
}

template <typename T>
void GenericTimebaseHandler<T>::Subscribe(std::weak_ptr<Consumer<T>> consumer, const Topic& topic)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_configured_, "Subscribe() must be called before Initialize()");
    broker_->AddSubscriber(topic, std::move(consumer));
}

template <typename T>
void GenericTimebaseHandler<T>::Initialize() noexcept
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_configured_, "Initialize() must be called only once");

    CreateComponents();

    init_job_runner_ = std::make_unique<JobRunner>(std::move(init_jobs_), name_ + "_init");
    is_configured_ = true;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": configured";
}

template <typename T>
void GenericTimebaseHandler<T>::RunOnce(const score::cpp::stop_token& token) noexcept
{
    if (token.stop_requested())
    {
        Stop();
        return;
    }

    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_configured_, "RunOnce() called before Initialize()");
    switch (handler_status_)
    {
        case TimebaseHandler::Status::kIdle:
        {
            init_job_runner_->Start(token);
            MoveToInitializeState();
            break;
        }
        case TimebaseHandler::Status::kInitialize:
        {
            PollInitialization(token);
            break;
        }
        case TimebaseHandler::Status::kWorking:
        case TimebaseHandler::Status::kFailed:
        {
            break;
        }
    }
}

template <typename T>
void GenericTimebaseHandler<T>::Stop() noexcept
{
    if (handler_status_ == TimebaseHandler::Status::kIdle)
    {
        return;
    }
    if (handler_status_ == TimebaseHandler::Status::kWorking)
    {
        for (auto it = start_order_.rbegin(); it != start_order_.rend(); ++it)
        {
            (*it)->Stop();
        }
        score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": proactive machines stopped";
    }
    MoveToIdleState();
}

template <typename T>
void GenericTimebaseHandler<T>::PollInitialization(const score::cpp::stop_token& token) noexcept
{
    const auto init_result = init_job_runner_->GetResult();
    if (token.stop_requested())
    {
        Stop();
        return;
    }

    switch (init_result)
    {
        case JobRunner::Result::kIdle:
        {
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(false, "Initialization worker has not been started");
            break;
        }
        case JobRunner::Result::kInProgress:
        {
            break;
        }
        case JobRunner::Result::kSucceed:
        {
            StartMachines();
            MoveToWorkingState();
            break;
        }
        case JobRunner::Result::kFailed:
        {
            MoveToFailedState();
            break;
        }
    }
}

template <typename T>
void GenericTimebaseHandler<T>::StartMachines() noexcept
{
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": starting proactive machines";
    for (const auto& machine : start_order_)
    {
        machine->Start();
    }
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToIdleState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kIdle;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": stopped";
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToInitializeState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kInitialize;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": initialization started";
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToWorkingState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kWorking;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": working";
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToFailedState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kFailed;
    score::mw::log::LogError(kTimeBaseHandlerGeneric) << name_ << ": initialization failed";
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H
