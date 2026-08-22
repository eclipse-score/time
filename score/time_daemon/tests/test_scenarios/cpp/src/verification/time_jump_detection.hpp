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
#ifndef SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_TIME_JUMP_DETECTION_HPP
#define SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_TIME_JUMP_DETECTION_HPP

#include <scenario.hpp>

// Verifies TimeJumpsValidator forward-jump detection inside SvtVerificationMachine.
// After the initial sync debouncing phase (5 s wall clock + 2 valid frames), a
// ptp_assumed_time jump exceeding max_time_jump_allowed (500 µs) must raise
// is_time_jump_future in the output.  This scenario takes ~6 s to run.
class TimeJumpDetection final : public Scenario
{
  public:
    ~TimeJumpDetection() final = default;

    std::string name() const final;
    void run(const std::string& input) const final;
};

#endif  // SCORE_TIME_DAEMON_TESTS_TEST_SCENARIOS_VERIFICATION_TIME_JUMP_DETECTION_HPP
