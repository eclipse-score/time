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
#ifndef SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_H
#define SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_H

#include <memory>

namespace score
{
namespace td
{

/**
 * @brief Abstract base class for validation stages in the verification pipeline.
 *
 * Each stage implements the Chain of Responsibility pattern, performing specific
 * validation and passing the result to the next stage in the pipeline.
 */
template <class DataType>
class VerificationStage
{
  public:
    /**
     * @brief Default constructor.
     */
    VerificationStage() = default;

    virtual ~VerificationStage() = default;

    /**
     * @brief Processes time information and passes to next stage.
     *
     * @param data The time information to process
     * @return The processed time information
     */
    auto Process(DataType data) -> DataType;

    /**
     * @brief Sets the next stage in the pipeline.
     *
     * @param next Pointer to the next verification stage
     */
    void SetNext(std::unique_ptr<VerificationStage<DataType>> next);

  protected:
    VerificationStage(const VerificationStage& other) = delete;
    VerificationStage& operator=(const VerificationStage& other) = delete;
    VerificationStage(VerificationStage&& other) = delete;
    VerificationStage& operator=(VerificationStage&& other) = delete;

    /**
     * @brief Performs the actual validation logic for this stage.
     *
     * Derived classes must implement this method to define their specific
     * validation behavior.
     *
     * @param data The time information to validate
     * @return The validated time information with any qualifiers added
     */
    virtual void DoValidation(DataType& data) = 0;

  private:
    /** @brief Pointer to the next stage in the pipeline */
    std::unique_ptr<VerificationStage<DataType>> next_stage_;
};

template <class DataType>
auto VerificationStage<DataType>::Process(DataType data) -> DataType
{
    DoValidation(data);

    if (next_stage_)
    {
        return next_stage_->Process(std::move(data));
    }

    return std::move(data);
}

template <class DataType>
void VerificationStage<DataType>::SetNext(std::unique_ptr<VerificationStage<DataType>> next)
{
    next_stage_ = std::move(next);
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_H
