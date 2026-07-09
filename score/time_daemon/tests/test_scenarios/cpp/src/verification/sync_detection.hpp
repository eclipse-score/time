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
#ifndef SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_SYNC_DETECTION_HPP
#define SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_SYNC_DETECTION_HPP

#include <scenario.hpp>

// Verifies SynchronizationValidator latching behavior inside SvtVerificationMachine
// by directly injecting PtpTimeInfo objects with different synchronization states.
class SyncDetection final : public Scenario
{
  public:
    ~SyncDetection() final = default;

    std::string name() const final;
    void run(const std::string& input) const final;
};

#endif  // SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_SYNC_DETECTION_HPP
