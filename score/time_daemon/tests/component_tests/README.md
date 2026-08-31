<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# TimeDaemon Component Integration Tests (CIT)

## Scope

Component integration tests verify **behaviour across multiple modules inside the
TimeDaemon component** — wiring, data flow, IPC, and lifecycle — from the
component's public surface.  They are NOT a replacement for unit tests.

| Layer | Tool | Where it lives | What it covers |
|-------|------|----------------|----------------|
| Unit | gtest / gmock | `score/time_daemon/src/**/*_test.cpp` | Individual classes and small modules, white-box |
| **CIT (this layer)** | **pytest + test_scenarios binary** | `score/time_daemon/tests/component_tests/` | **Multi-module integration, cross-module data flow, IPC, lifecycle** |
| Module integration | pytest + ITF | `score/tests/module_integration_tests/` | TimeSlave ↔ TimeDaemon across components |
| System | pytest + ITF + QEMU / Docker | `score/tests/gptp_pipeline/` | Multi-binary end-to-end across OS targets |

**Rule of thumb** — if a scenario only instantiates a single production class and
exercises its public API, it belongs in the gtest unit layer under `src/`, not
here.  A CIT scenario should exercise **two or more production objects wired
together** (callback chaining, message-bus topics, shared-memory IPC, full daemon
startup, etc.).

## Architecture

```
pytest (Python)                test runner, assertions, reporting
    │
    ▼  subprocess + structured-log parsing
test_scenarios (C++ binary)    scenario runner, builds real production objects
    │
    ▼  factory + public API
TimeDaemon production code     classes under test
```

- Python side lives under `tests/` and subclasses `testing_utils.Scenario`.
- C++ side lives under `../test_scenarios/cpp/` and uses the
  `@score_test_scenarios` framework (`Scenario`, `TRACING_INFO`, CLI runner).
- Each pytest class declares a `scenario_name` fixture that selects which
  C++ scenario to run; results come back as exit code + structured JSON log lines
  parsed by `testing_utils.LogContainer`.

## Scenarios

| Scenario name | Modules involved | What it verifies |
|---------------|-----------------|------------------|
| `verification.pipeline` | `GPTPStubMachine` → `SvtVerificationMachine` (3 validators) | PTP data flows correctly through the full verification pipeline |
| `ipc.shm_roundtrip` | `SvtPublisher` ↔ `SvtReceiver` via shared memory | Shared-memory IPC round-trip preserves data integrity |
| `daemon.lifecycle` | `SvtHandler` + all subsystems (MessageBroker, machines, IPC) | Daemon initialises and shuts down cleanly through all lifecycle states |

## Running

```bash
bazel test //score/time_daemon/tests/component_tests:time_daemon_cit
```
