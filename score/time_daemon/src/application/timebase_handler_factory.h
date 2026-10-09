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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLER_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_TIMEBASE_HANDLER_FACTORY_H

#include "score/time_daemon/src/application/timebase_handler.h"

#include <memory>
#include <vector>

namespace score
{
namespace td
{

/// @brief Creates the complete handler composition, defined by exactly one linked provider.
std::vector<std::unique_ptr<TimebaseHandler>> CreateTimebaseHandlers();

}  // namespace td
}  // namespace score

#endif
