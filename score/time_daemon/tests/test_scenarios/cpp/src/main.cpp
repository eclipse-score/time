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
#include <iostream>
#include <string>
#include <vector>

#include <cli.hpp>
#include <scenario.hpp>
#include <test_context.hpp>

#include "control_flow/ptp_divider.hpp"
#include "daemon/lifecycle.hpp"
#include "ipc/shm_roundtrip.hpp"
#include "svt/stub_sync.hpp"
#include "verification/pipeline.hpp"

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> raw_arguments{argv, argv + argc};

        ScenarioGroup::Ptr svt_group{svt_scenario_group()};
        ScenarioGroup::Ptr verification_group{verification_scenario_group()};
        ScenarioGroup::Ptr control_flow_group{control_flow_scenario_group()};
        ScenarioGroup::Ptr daemon_group{daemon_scenario_group()};
        ScenarioGroup::Ptr ipc_group{ipc_scenario_group()};
        ScenarioGroup::Ptr root_group{new ScenarioGroupImpl{
            "root", {}, {svt_group, verification_group, control_flow_group, daemon_group, ipc_group}}};

        TestContext test_context{root_group};
        run_cli_app(raw_arguments, test_context);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 101;
    }
}
