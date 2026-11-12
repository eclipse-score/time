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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_GENERIC_GENERIC_TIMEBASE_HANDLER_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_GENERIC_GENERIC_TIMEBASE_HANDLER_H

#include "score/mw/log/logging.h"
#include "score/time_daemon/src/application/job_runner/job_runner.h"
#include "score/time_daemon/src/application/timebase_handlers/timebase_handler.h"
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

/// \brief Timebase handler that owns a MessageBroker for communication between its machines.
///
/// A derived handler adds machines in CreateComponents(). Use Publish() and Subscribe() to connect them.
/// Do not add or connect machines after Initialize().
///
/// \tparam T Type of messages sent through the MessageBroker.
template <typename T>
class GenericTimebaseHandler : public TimebaseHandler
{
  public:
    /// \brief Stops the timebase handler before destruction.
    ~GenericTimebaseHandler() noexcept override;

    /// \brief Initializes the timebase handler. Call once, before Start().
    void Initialize() noexcept override;

    /// \brief Starts the timebase handler. Call after Initialize().
    /// \param token Stop token used to cancel initialization.
    void Start(const score::cpp::stop_token& token) noexcept override;

    /// \brief Performs one iteration of timebase handler processing. Call after Initialize().
    void RunOnce() noexcept override;

    /// \brief Stops the timebase handler.
    void Stop() noexcept override;

    /// \brief Returns true if initialization failed, e.g. timed out. Not set by a requested stop.
    bool IsFailed() const noexcept override;

  protected:
    /// \param name Name used in log messages and for the initialization thread.
    explicit GenericTimebaseHandler(std::string name);

    /// \brief Adds a machine with an initialization timeout. A timeout makes the handler fail.
    /// \return The machine passed to Add().
    template <typename M>
    std::shared_ptr<M> Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout);

    /// \brief Sets the producer's topic. Each producer can use only one topic.
    void Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic);

    /// \brief Registers the consumer for messages published on the topic.
    void Subscribe(std::weak_ptr<Consumer<T>> consumer, const Topic& topic);

  private:
    /// \brief Adds and connects machines. Initialize() calls this once.
    virtual void CreateComponents() noexcept = 0;

    void PollInitialization() noexcept;
    void StartMachines() noexcept;
    void MoveToStartingState() noexcept;
    void MoveToWorkingState() noexcept;
    void MoveToStoppedState() noexcept;

    std::unique_ptr<MessageBroker<T>> broker_;
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
    /// Set when initialization failed on its own, not because a stop was requested.
    bool is_failed_;
    score::cpp::stop_token stop_token_;
};

template <typename T>
GenericTimebaseHandler<T>::GenericTimebaseHandler(std::string name)
    : broker_{std::make_unique<MessageBroker<T>>()},
      machines_{},
      start_order_{},
      name_{std::move(name)},
      init_jobs_{},
      producers_{},
      init_job_runner_{nullptr},
      handler_status_{TimebaseHandler::Status::kIdle},
      is_configured_{false},
      is_failed_{false},
      stop_token_{}
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
void GenericTimebaseHandler<T>::RunOnce() noexcept
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_configured_, "RunOnce() called before Initialize()");
    switch (handler_status_)
    {
        case TimebaseHandler::Status::kIdle:
        {
            // Timebase handler idle
            break;
        }
        case TimebaseHandler::Status::kStarting:
        {
            PollInitialization();
            break;
        }
        case TimebaseHandler::Status::kWorking:
        {
            // Timebase handler is running normally. Machines are started.
            break;
        }
        case TimebaseHandler::Status::kStopped:
        {
            // Machines stopped.
            break;
        }
    }
}

template <typename T>
void GenericTimebaseHandler<T>::Start(const score::cpp::stop_token& token) noexcept
{
    stop_token_ = token;
    init_job_runner_->Start(token);
    MoveToStartingState();
}

template <typename T>
void GenericTimebaseHandler<T>::Stop() noexcept
{
    for (auto it = start_order_.rbegin(); it != start_order_.rend(); ++it)
    {
        (*it)->Stop();
    }
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": proactive machines stopped";
    MoveToStoppedState();
}

template <typename T>
bool GenericTimebaseHandler<T>::IsFailed() const noexcept
{
    return is_failed_;
}

template <typename T>
void GenericTimebaseHandler<T>::PollInitialization() noexcept
{
    const auto init_result = init_job_runner_->GetResult();
    switch (init_result)
    {
        case JobRunner::Result::kIdle:
        {
            score::mw::log::LogInfo(kTimeBaseHandlerGeneric)
                << name_ << ": " << "Initialization worker has not been started yet";
            break;
        }
        case JobRunner::Result::kInProgress:
        {
            score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": " << "Initialization in progress";
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
            score::mw::log::LogError(kTimeBaseHandlerGeneric) << name_ << ": " << "Initialization failed";
            // The job runner also reports failure when a stop was requested during initialization.
            is_failed_ = !stop_token_.stop_requested();
            Stop();
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
void GenericTimebaseHandler<T>::MoveToStartingState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kStarting;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": starting";
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToWorkingState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kWorking;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": working";
}

template <typename T>
void GenericTimebaseHandler<T>::MoveToStoppedState() noexcept
{
    handler_status_ = TimebaseHandler::Status::kStopped;
    score::mw::log::LogError(kTimeBaseHandlerGeneric) << name_ << ": stopped";
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLERS_GENERIC_GENERIC_TIMEBASE_HANDLER_H
