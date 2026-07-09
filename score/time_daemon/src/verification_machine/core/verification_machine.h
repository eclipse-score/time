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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_MACHINE_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_MACHINE_H

#include "score/mw/log/logging.h"
#include "score/time_daemon/src/common/data_flow/consumer.h"
#include "score/time_daemon/src/common/data_flow/producer.h"
#include "score/time_daemon/src/common/logging_contexts.h"
#include "score/time_daemon/src/common/machines/reactive_machine.h"
#include "score/time_daemon/src/verification_machine/core/verification_stage.h"

#include <memory>
#include <string>
#include <vector>

namespace score
{
namespace td
{

/**
 * @brief Machine component responsible for validating and qualifying time information.
 *
 * The VerificationMachine implements a pipeline pattern where each stage performs
 * specific validation and adds appropriate qualifiers to the provided data.
 */
template <typename DataType>
class VerificationMachine final : public ReactiveMachine, public Consumer<DataType>, public Producer<DataType>
{
  public:
    /**
     * @brief Constructor for VerificationMachine with validator factories.
     *
     * Each factory creates one validation stage. Stages are processed in the order
     * they are provided (first factory creates first stage in pipeline).
     *
     * @tparam Factories Types of factory functions
     * @param name The name of this verification machine instance
     * @param factories Factory functions to create each validator. Each factory
     *        must return a unique_ptr to a VerificationStage<DataType>.
     *        Factories can be lambdas, std::bind expressions, or function pointers.
     */
    template <typename... Factories>
    explicit VerificationMachine(const std::string& name, Factories&&... factories)
        : ReactiveMachine(name), Consumer<DataType>(), Producer<DataType>(), pipeline_(), publish_callback_()
    {
        std::vector<StageFactory> factories_container;
        // Store each factory in the array
        (factories_container.push_back(StageFactory(std::forward<Factories>(factories))), ...);

        SetupPipeline(factories_container);
    }

    VerificationMachine(const VerificationMachine&) = delete;
    VerificationMachine& operator=(const VerificationMachine&) = delete;
    VerificationMachine(VerificationMachine&&) = delete;
    VerificationMachine& operator=(VerificationMachine&&) = delete;
    ~VerificationMachine() override = default;

    /**
     * @brief Sets the callback function to be invoked when publishing data.
     *
     * This method allows external components (typically the message broker)
     * to register a callback that will be invoked when this producer publishes
     * data.
     *
     * @param callback Function to be called when data is published
     */
    void SetPublishCallback(std::function<void(const DataType&)> callback) override;

    /**
     * @brief Process the received time information.
     *
     * This method is responsible for processing the time information data.
     *
     * @param data The time information data to be processed
     */
    void OnMessage(DataType data) override;

    /**
     * @brief Initialize machine
     *
     * As there is no explicit Init actions, it will be stubbed and return true.
     *
     * @param bool Init result
     */
    bool Init() override;

  private:
    // Factory function type for creating validator stages with custom arguments
    using Stage = VerificationStage<DataType>;
    using StagePtr = std::unique_ptr<Stage>;
    using StageFactory = std::function<StagePtr()>;

    /**
     * @brief Publishes the time information data using the registered callback.
     *
     * This method invokes the previously set publish callback to distribute
     * the data to interested consumers.
     *
     * @param data The data to be published
     */
    void Publish(const DataType& data) override;

    /**
     * @brief Sets up the validation pipeline by creating and connecting stages.
     */
    void SetupPipeline(const std::vector<StageFactory>& factories);

    /**
     * @brief Processes time information through the validation pipeline.
     *
     * @param data The time information to validate
     * @return The validated and qualified time information
     */
    auto ProcessMessage(DataType data) -> DataType;

    /** @brief First stage in the validation pipeline */
    std::unique_ptr<Stage> pipeline_;

    std::function<void(const DataType&)> publish_callback_;
};

template <typename DataType>
void VerificationMachine<DataType>::SetPublishCallback(std::function<void(const DataType&)> callback)
{
    publish_callback_ = std::move(callback);
}

template <typename DataType>
void VerificationMachine<DataType>::Publish(const DataType& data)
{
    if (publish_callback_)
    {
        score::mw::log::LogDebug(kVerificationMachineContext) << "Publishing data " << data;
        publish_callback_(data);
    }
    else
    {
        score::mw::log::LogWarn(kVerificationMachineContext) << "Publish callback not set, cannot publish data";
    }
}

template <typename DataType>
void VerificationMachine<DataType>::OnMessage(DataType data)
{
    score::mw::log::LogDebug(kVerificationMachineContext) << "Receive new data " << data;
    auto processed = ProcessMessage(std::move(data));
    Publish(processed);
}

template <typename DataType>
bool VerificationMachine<DataType>::Init()
{
    return true;
}

template <typename DataType>
auto VerificationMachine<DataType>::ProcessMessage(DataType data) -> DataType
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(pipeline_ != nullptr,
                                                "ProcessMessage shall only be called when pipeline is setup");
    return pipeline_->Process(std::move(data));
}

template <typename DataType>
void VerificationMachine<DataType>::SetupPipeline(const std::vector<StageFactory>& factories)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(!factories.empty(),
                                                "SetupPipeline shall be called with provided stage factories");

    std::vector<StagePtr> stages;
    stages.reserve(factories.size());

    // Create all stages using factories
    for (const auto& factory : factories)
    {
        auto stage = factory();
        SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD_MESSAGE(stage != nullptr, "Validator factory returned nullptr");
        stages.push_back(std::move(stage));
    }

    // Connect from back to front not to touch already moved elements.
    // (rbegin points to last element; we stop before the original first (stages.size() - 1))
    for (size_t i = stages.size() - 1U; i > 0U; --i)
    {
        stages[i - 1U]->SetNext(std::move(stages[i]));
    }

    pipeline_ = std::move(stages[0]);
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_MACHINE_H
