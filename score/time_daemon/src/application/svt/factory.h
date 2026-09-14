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
#ifndef SCORE_TIME_DAEMON_SRC_APPLICATION_FACTORY_H
#define SCORE_TIME_DAEMON_SRC_APPLICATION_FACTORY_H

#include "score/time_daemon/src/application/timebase_handler.h"

#include <memory>

namespace score::td
{

/// \brief Creates a new SVT timebase handler
///
/// \return std::unique_ptr<TimebaseHandler> New SVT timebase handler
auto CreateSvtTimebase() -> std::unique_ptr<TimebaseHandler>;

}  // namespace score::td

#endif  // SCORE_TIME_DAEMON_SRC_APPLICATION_FACTORY_H
