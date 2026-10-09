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

/// \brief Timebase handler assembled from machines and topics.
///
/// Generalizes the lifecycle of SvtHandler to any timebase:
/// - Add() registers a machine. Init() runs asynchronously in a JobRunner, bounded by the machine's timeout.
/// - Publish() and Subscribe() declare the topic wiring. Initialize() applies it, subscribers first.
/// - RunOnce() drives the states kIdle -> kInitialize -> kWorking, or kFailed.
/// - ProactiveMachines start in Add() order and stop in reverse order. Add the machines that must run first
///   before the machines that publish data.
///
/// The handler owns all added machines, because the MessageBroker only holds weak references to them.
///
/// \tparam T Message type exchanged over the MessageBroker, e.g. PtpTimeInfo.
template <typename T>
class GenericTimebaseHandler final : public TimebaseHandler
{
  public:
    /// \brief Creates a handler without machines and topics.
    ///
    /// \param name Name of the handler. Used for the JobRunner thread name and for logging.
    explicit GenericTimebaseHandler(std::string name);

    /// \brief Registers a machine. The handler initializes it, and starts it if it is a ProactiveMachine.
    ///
    /// Init() is retried asynchronously in a JobRunner until it succeeds or init_timeout expires. If it expires,
    /// the handler fails and starts no machine.
    ///
    /// \pre Called before Initialize(), with a non-null machine.
    /// \param machine      Machine to register. The handler keeps it alive.
    /// \param init_timeout Time after which the initialization of this machine has failed.
    /// \return The registered machine, so that the caller keeps typed access to it.
    template <typename M>
    std::shared_ptr<M> Add(std::shared_ptr<M> machine, std::chrono::seconds init_timeout = std::chrono::seconds{20});

    /// \brief Declares that a producer publishes on a topic. The binding is applied in Initialize().
    ///
    /// \pre Called before Initialize(). A producer publishes on one topic only, a second binding asserts.
    /// \param producer Producer that publishes on the topic.
    /// \param topic    Topic to publish on.
    void Publish(std::weak_ptr<Producer<T>> producer, const Topic& topic);

    /// \brief Declares that a consumer receives the messages of a topic. The binding is applied in Initialize().
    ///
    /// \pre Called before Initialize().
    /// \param topic    Topic to subscribe to.
    /// \param consumer Consumer that receives the messages.
    void Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer);

    /// \brief Applies the topic wiring to the MessageBroker. Called once, before RunOnce().
    void Initialize() noexcept override;

    /// \brief Starts the init jobs on the first call. Starts the ProactiveMachines once the jobs have succeeded.
    /// \param token Stop token passed to the JobRunner, which refers to it while the init jobs run.
    ///              It must outlive the initialization.
    void RunOnce(const score::cpp::stop_token& token) noexcept override;

    /// \brief Stops the ProactiveMachines in reverse start order. Does nothing unless the handler is working.
    void Stop() noexcept override;

  private:
    std::shared_ptr<MessageBroker<T>> broker_;
    std::vector<std::shared_ptr<BaseMachine>> machines_;          // owns all added machines
    std::vector<std::shared_ptr<ProactiveMachine>> start_order_;  // ProactiveMachines in Add() order
    std::string name_;
    /// Init jobs of all added machines. Moved into the JobRunner by Initialize().
    std::vector<Job> init_jobs_;

    /// Topic and producer, applied to the MessageBroker in Initialize().
    struct ProducerBinding
    {
        Topic topic;
        std::weak_ptr<Producer<T>> producer;
    };

    /// Topic and consumer, applied to the MessageBroker in Initialize().
    struct ConsumerBinding
    {
        Topic topic;
        std::weak_ptr<Consumer<T>> consumer;
    };

    /// Returns true if the producer already has a binding. Compares ownership, so expired producers match too.
    bool IsProducerBound(const std::weak_ptr<Producer<T>>& producer) const noexcept;

    std::vector<ProducerBinding> producer_bindings_;
    std::vector<ConsumerBinding> consumer_bindings_;
    /// Runs the init jobs. Created in Initialize(), released once the jobs have succeeded.
    std::unique_ptr<JobRunner> job_runner_;
    /// Current state of the handler.
    TimebaseHandler::Status handler_status_;
    /// Set by Initialize(). Guards the registration API.
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
    producer_bindings_.push_back(ProducerBinding{topic, std::move(producer)});
}

template <typename T>
void GenericTimebaseHandler<T>::Subscribe(const Topic& topic, std::weak_ptr<Consumer<T>> consumer)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Subscribe() must be called before Initialize()");
    consumer_bindings_.push_back(ConsumerBinding{topic, std::move(consumer)});
}

template <typename T>
bool GenericTimebaseHandler<T>::IsProducerBound(const std::weak_ptr<Producer<T>>& producer) const noexcept
{
    const auto is_same_producer = [&producer](const ProducerBinding& binding) {
        return !binding.producer.owner_before(producer) && !producer.owner_before(binding.producer);
    };
    return std::any_of(producer_bindings_.cbegin(), producer_bindings_.cend(), is_same_producer);
}

template <typename T>
void GenericTimebaseHandler<T>::Initialize() noexcept
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!is_initialized_, "Initialize() must be called only once");

    // Subscribers first, so that no producer publishes into a topic without listeners.
    for (const auto& binding : consumer_bindings_)
    {
        broker_->AddSubscriber(binding.topic, binding.consumer);
    }
    for (const auto& binding : producer_bindings_)
    {
        broker_->AddProducer(binding.topic, binding.producer);
    }

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
      producer_bindings_{},
      consumer_bindings_{},
      job_runner_{nullptr},
      handler_status_{TimebaseHandler::Status::kIdle},
      is_initialized_{false}
{
    score::mw::log::LogInfo(kTimeBaseHandlerGeneric) << name_ << ": handler created";
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_GENERIC_GENERIC_TIMEBASE_HANDLER_H
