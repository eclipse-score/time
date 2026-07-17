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
#ifndef SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_ACQUISITION_HPP
#define SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_ACQUISITION_HPP

#include <scenario.hpp>

// Verifies that GptpEngine transitions to is_synchronized=true after receiving
// one valid Sync+FollowUp frame pair injected via a software-only FakeSocket.
class SyncAcquisition final : public Scenario
{
  public:
    ~SyncAcquisition() final = default;

    std::string name() const final;
    void run(const std::string& input) const final;
};

#endif  // SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_ACQUISITION_HPP
