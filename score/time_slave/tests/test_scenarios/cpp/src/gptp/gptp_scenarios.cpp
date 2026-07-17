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
#include "gptp/gptp_scenarios.hpp"
#include "gptp/sync_acquisition.hpp"
#include "gptp/sync_timeout.hpp"
#include "gptp/time_jump_detection.hpp"

#include <scenario.hpp>

ScenarioGroup::Ptr gptp_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{
        "gptp",
        {std::make_shared<SyncAcquisition>(), std::make_shared<SyncTimeout>(), std::make_shared<TimeJumpDetection>()},
        {}}};
}
