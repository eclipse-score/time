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
#ifndef SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_MOCK_H
#define SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_MOCK_H

#include "gmock/gmock.h"
#include "score/time_daemon/src/verification_machine/core/verification_stage.h"
#include <gtest/gtest.h>

#include <array>

namespace score
{
namespace td
{

struct ValidatorMockData
{
    int data[10] = {0};
};

template <typename OutputStream>
auto& operator<<(OutputStream& output_stream, const ValidatorMockData& data)
{
    std::for_each(std::begin(data.data), std::end(data.data), [&](const auto& value) {
        output_stream << value << " ";
    });

    return output_stream;
}

class VerificationStageMock : public VerificationStage<ValidatorMockData>
{
  public:
    VerificationStageMock() {}

    MOCK_METHOD(void, DoValidation, (ValidatorMockData&), (override));

  protected:
    size_t id_{0};
};

}  // namespace td
}  // namespace score

#endif  // SCORE_TIME_DAEMON_SRC_VERIFICATION_MACHINE_CORE_VERIFICATION_STAGE_MOCK_H
