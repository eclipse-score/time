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
#ifndef SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_TIMEOUT_HPP
#define SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_TIMEOUT_HPP

#include <scenario.hpp>

// Verifies that GptpEngine sets is_timeout=true and clears is_synchronized
// when no new Sync frames arrive within the configured reception window.
// Uses a short timeout_ms (100 ms) to keep the CIT fast.
class SyncTimeout final : public Scenario
{
  public:
    ~SyncTimeout() final = default;

    std::string name() const final;
    void run(const std::string& input) const final;
};

#endif  // SCORE_TIME_SLAVE_TESTS_TEST_SCENARIOS_GPTP_SYNC_TIMEOUT_HPP
