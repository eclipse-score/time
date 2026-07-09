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
#include "daemon/lifecycle.hpp"

#include <string>

#include <scenario.hpp>
#include <tracing.hpp>

#include "score/time_daemon/src/application/svt/factory.h"

namespace
{
const std::string kTargetName{"test_scenarios::daemon::lifecycle"};
}

// Verifies that the TimeDaemon subsystem stack can be constructed, initialized,
// and stopped cleanly.  Exercises all four component factory functions
// (GPTPStubMachine, SvtVerificationMachine, IpcPublisher, PtpControlFlowDivider)
// and the full MessageBroker pub-sub topology wiring performed by SvtHandler::Initialize().
// Stop() is called from the kIdle state (no async work started), confirming that
// the graceful-shutdown path is a safe no-op before any subsystem has been started.
class DaemonLifecycle final : public Scenario
{
  public:
    ~DaemonLifecycle() final = default;

    std::string name() const final
    {
        return "lifecycle";
    }

    void run(const std::string& /*input*/) const final
    {
        // Phase 1: construct — calls all four subsystem factory functions
        auto handler = score::td::CreateSvtTimebase();

        // Phase 2: initialize — wires MessageBroker pub-sub topology
        handler->Initialize();

        // Phase 3: stop from kIdle — verifies graceful shutdown without starting async workers
        handler->Stop();

        TRACING_INFO(kTargetName,
                     std::pair{std::string{"lifecycle_initialize_ok"}, std::string{"true"}},
                     std::pair{std::string{"lifecycle_complete"}, std::string{"true"}});
    }
};

ScenarioGroup::Ptr daemon_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{
        "daemon",
        {std::make_shared<DaemonLifecycle>()},
        {}}};
}
