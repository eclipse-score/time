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
/// 	param T Type of messages sent through the MessageBroker.
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
    std::shared_ptr<M> Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout = std::chrono::seconds{20});

    /// \brief Sets the producer's topic. Each producer can use only one topic.
    void Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic);

    /// \brief Sends messages from the topic to the consumer.
    void Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer);

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

    std::shared_ptr<MessageBroker<T>> broker_;
    /// Keeps the machines alive. The MessageBroker holds only weak references.
    std::vector<std::shared_ptr<BaseMachine>> machines_;
    std::vector<std::shared_ptr<ProactiveMachine>> start_order_;  // in Add() order
    std::string name_;
    std::vector<Job> init_jobs_;
    /// Tracks producers to reject duplicate Publish() calls.
    std::set<std::weak_ptr<Producer<T>>, std::owner_less<std::weak_ptr<Producer<T>>>> producers_;
    /// Kept until the handler is destroyed.
    std::unique_ptr<JobRunner> job_runner_;
    TimebaseHandler::Status handler_status_;
    bool is_initialized_;
};

template <typename T>
template <typename M>
std::shared_ptr<M> GenericTimebaseHandler<T>::Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Machines must be added before Initialize()");
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
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Publish() must be called before Initialize()");
    const auto is_new_producer = producers_.insert(producer).second;
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_new_producer, "A producer publishes on one topic only");
    broker_->AddProducer(topic, std::move(producer));
}

template <typename T>
void GenericTimebaseHandler<T>::Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Subscribe() must be called before Initialize()");
    broker_->AddSubscriber(topic, std::move(consumer));
}

template <typename T>
void GenericTimebaseHandler<T>::Initialize() noexcept
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Initialize() must be called only once");

    CreateComponents();

    job_runner_ = std::make_unique<JobRunner>(std::move(init_jobs_), name_ + "_init");
    is_initialized_ = true;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": initialized";
}

template <typename T>
void GenericTimebaseHandler<T>::RunOnce(const score::cpp::stop_token& token) noexcept
{
    if (token.stop_requested())
    {
        Stop();
        return;
    }

    switch (handler_status_)
    {
        case TimebaseHandler::Status::kIdle:
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_initialized_, "RunOnce() called before Initialize()");
            job_runner_->Start(token);
            handler_status_ = TimebaseHandler::Status::kInitialize;
            score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": initialization started";
            break;
        case TimebaseHandler::Status::kInitialize:
            PollInitialization(token);
            break;
        case TimebaseHandler::Status::kWorking:
        case TimebaseHandler::Status::kFailed:
            break;
    }
}

template <typename T>
void GenericTimebaseHandler<T>::PollInitialization(const score::cpp::stop_token& token) noexcept
{
    const auto result = job_runner_->GetResult();
    if (token.stop_requested())
    {
        Stop();
        return;
    }

    switch (result)
    {
        case JobRunner::Result::kIdle:
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(false, "Initialization worker has not been started");
            return;
        case JobRunner::Result::kInProgress:
            return;
        case JobRunner::Result::kSucceed:
            StartMachines();
            return;
        case JobRunner::Result::kFailed:
            handler_status_ = TimebaseHandler::Status::kFailed;
            score::mw::log::LogError(kTimeBaseHandlerGeneric) << name_ << ": initialization failed";
            return;
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
    handler_status_ = TimebaseHandler::Status::kWorking;
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": working";
}

template <typename T>
void GenericTimebaseHandler<T>::Stop() noexcept
{
    if (handler_status_ == TimebaseHandler::Status::kWorking)
    {
        for (auto it = start_order_.rbegin(); it != start_order_.rend(); ++it)
        {
            (*it)->Stop();
        }
        score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": proactive machines stopped";
    }
    handler_status_ = TimebaseHandler::Status::kIdle;
}

template <typename T>
GenericTimebaseHandler<T>::GenericTimebaseHandler(std::string name)
    : broker_{std::make_shared<MessageBroker<T>>()},
      machines_{},
      start_order_{},
      name_{std::move(name)},
      init_jobs_{},
      producers_{},
      job_runner_{nullptr},
      handler_status_{TimebaseHandler::Status::kIdle},
      is_initialized_{false}
{
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": handler created";
}

template <typename T>
GenericTimebaseHandler<T>::~GenericTimebaseHandler() noexcept
{
    Stop();
    job_runner_.reset();
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H
