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
#ifndef SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_FACTORY_SHM_H
#define SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_FACTORY_SHM_H

#include "score/TimeDaemon/code/application/timebase_handler.h"

#include <memory>

namespace score
{
namespace td
{

/// \brief Creates an SVT timebase handler backed by the shared-memory gPTP engine.
///
/// Reads live gPTP data written by TimeSlave via GptpIpcPublisher.
/// Use in place of CreateSvtTimebase() when running alongside a real TimeSlave.
std::unique_ptr<TimebaseHandler> CreateSvtTimebaseShm();

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_APPLICATION_SVT_FACTORY_SHM_H
