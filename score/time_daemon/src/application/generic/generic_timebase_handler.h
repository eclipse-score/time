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

#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace score
{
namespace td
{

/// \brief Timebase handler built from machines that exchange messages through a MessageBroker.
///
/// A derived handler implements CreateComponents() to Add() its machines and to declare their wiring with
/// Publish() and Subscribe(). None of these may be called after Initialize().
///
/// The machines are initialized asynchronously in a JobRunner. Once all of them are initialized, the
/// ProactiveMachines are started in the order they were added, and stopped in reverse order. Hence add the machines
/// that must run first before the machines that publish data.
///
/// \tparam T Message type exchanged through the MessageBroker.
template <typename T>
class GenericTimebaseHandler : public TimebaseHandler
{
  public:
    /// \param name Name used in log messages and for the initialization thread.
    explicit GenericTimebaseHandler(std::string name);

    /// \brief Registers a machine. Its Init() is retried in the JobRunner until it succeeds.
    /// The initialization of the handler fails if init_timeout expires first.
    /// \return The registered machine, so that the caller keeps typed access to it.
    template <typename M>
    std::shared_ptr<M> Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout = std::chrono::seconds{20});

    /// \brief Declares the topic a producer publishes on. A producer publishes on one topic only.
    void Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic);

    /// \brief Declares that a consumer receives the messages of a topic.
    void Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer);

    /// \brief Calls CreateComponents() and prepares the initialization. Called once, before RunOnce().
    void Initialize() noexcept override;

    /// \brief Starts the initialization on the first call, and the ProactiveMachines once it has succeeded.
    /// \param token Must outlive the initialization, because the JobRunner refers to it.
    void RunOnce(const score::cpp::stop_token& token) noexcept override;

    /// \brief Stops the ProactiveMachines in reverse start order, if they were started.
    void Stop() noexcept override;

  protected:
    /// \brief Adds the machines and declares their wiring. Called once by Initialize().
    virtual void CreateComponents() noexcept = 0;

  private:
    /// Compares ownership, so that expired producers match too.
    bool IsProducerBound(const std::weak_ptr<Producer<T>>& producer) const noexcept;

    std::shared_ptr<MessageBroker<T>> broker_;
    /// Owns the machines, because the MessageBroker only holds weak references.
    std::vector<std::shared_ptr<BaseMachine>> machines_;
    std::vector<std::shared_ptr<ProactiveMachine>> start_order_;  // in Add() order
    std::string name_;
    /// Moved into the JobRunner by Initialize().
    std::vector<Job> init_jobs_;
    /// Only used to reject a producer that is published twice.
    std::vector<std::weak_ptr<Producer<T>>> producers_;
    /// Exists from Initialize() until the initialization has succeeded.
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
    if constexpr (std::is_base_of<ProactiveMachine, M>::value)
    {
        start_order_.push_back(machine);
    }
    return machine;
}

template <typename T>
void GenericTimebaseHandler<T>::Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Publish() must be called before Initialize()");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!IsProducerBound(producer), "A producer publishes on one topic only");
    broker_->AddProducer(topic, producer);
    producers_.push_back(std::move(producer));
}

template <typename T>
void GenericTimebaseHandler<T>::Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Subscribe() must be called before Initialize()");
    broker_->AddSubscriber(topic, std::move(consumer));
}

template <typename T>
bool GenericTimebaseHandler<T>::IsProducerBound(const std::weak_ptr<Producer<T>>& producer) const noexcept
{
    return std::any_of(producers_.cbegin(), producers_.cend(), [&producer](const auto& bound) {
        return !bound.owner_before(producer) && !producer.owner_before(bound);
    });
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
    switch (handler_status_)
    {
        case TimebaseHandler::Status::kIdle:
        {
            SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(is_initialized_, "RunOnce() called before Initialize()");
            job_runner_->Start(token);
            handler_status_ = TimebaseHandler::Status::kInitialize;
            score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": initialization started";
            break;
        }
        case TimebaseHandler::Status::kInitialize:
        {
            switch (job_runner_->GetResult())
            {
                case JobRunner::Result::kSucceed:
                {
                    job_runner_.reset();
                    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": starting proactive machines";
                    for (const auto& machine : start_order_)
                    {
                        machine->Start();
                    }
                    handler_status_ = TimebaseHandler::Status::kWorking;
                    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": working";
                    break;
                }
                case JobRunner::Result::kFailed:
                {
                    handler_status_ = TimebaseHandler::Status::kFailed;
                    score::mw::log::LogError(kTimeBaseHandlerGeneric) << name_ << ": initialization failed";
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

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H
