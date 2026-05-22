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
#ifndef SCORE_TIME_SLAVE_SRC_COMMON_LOGGING_CONTEXTS_H
#define SCORE_TIME_SLAVE_SRC_COMMON_LOGGING_CONTEXTS_H

namespace score
{
namespace ts
{

/// Logging context for the gPTP protocol engine (RxThread / PdelayThread).
constexpr auto kGPtpMachineContext = "GTPS";

/// Logging context for the TimeSlave application lifecycle (Initialize / Run).
constexpr auto kTimeSlaveAppContext = "TSAP";

}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_SRC_COMMON_LOGGING_CONTEXTS_H
