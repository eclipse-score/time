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
#include "score/time_daemon/src/application/timebase_handler_factory.h"

#include "score/time_daemon/src/application/svt/factory.h"

#include <memory>
#include <vector>

namespace score
{
namespace td
{

std::vector<std::unique_ptr<TimebaseHandler>> CreateTimebaseHandlers()
{
    std::vector<std::unique_ptr<TimebaseHandler>> handlers;
    handlers.push_back(CreateSvtTimebase());
    return handlers;
}

}  // namespace td
}  // namespace score
