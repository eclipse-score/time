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
#ifndef SCORE_TIMEDAEMON_CODE_PTP_MACHINE_STUB_GPTP_STUB_MACHINE_H
#define SCORE_TIMEDAEMON_CODE_PTP_MACHINE_STUB_GPTP_STUB_MACHINE_H

#include "score/time_daemon/src/ptp_machine/core/ptp_machine.h"
#include "score/time_daemon/src/ptp_machine/stub/details/stub_ptp_engine.h"

#include "score/mw/log/logging.h"

#include <memory>

#include <chrono>
#include <optional>
#include <utility>

namespace score
{
namespace td
{

using GPTPStubMachine = PTPMachine<details::StubPTPEngine>;

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_PTP_MACHINE_STUB_GPTP_STUB_MACHINE_H
