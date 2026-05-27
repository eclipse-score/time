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
#include "score/time_daemon/src/application/svt/factory.h"

#include "score/time_daemon/src/application/svt/svt_handler.h"

namespace score
{
namespace td
{

std::unique_ptr<TimebaseHandler> CreateSvtTimebase()
{
    return std::make_unique<SvtHandler>();
}

}  // namespace td
}  // namespace score
