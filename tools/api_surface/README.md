<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Public C++ API Surface Gate

This package detects accidental changes to the public C++ API of the Time module.
It uses the shared S-CORE tool
[`@score_tooling//api_surface`](https://github.com/eclipse-score/tooling/tree/main/api_surface):
clang dumps the AST of the public headers of a target, the public symbols are
extracted, and the result is compared with a JSON lock file committed here.

Any difference fails the test: removed or renamed symbols, changed signatures and
qualifiers, and added symbols. Implementation changes (`.cpp` files, private
members, comments) do not change the extracted API and pass.

## Scope

The public API is defined by Bazel visibility: every C++ library alias with
`//visibility:public` has its own test and lock file `<name>.lock.json`. The
packages whose headers those aliases expose to consumers are covered as well.
[`BUILD`](BUILD) lists the covered targets:

| Test / lock file | Covered target | API |
|------------------|----------------|-----|
| `clock` | `//score/time/clock:clock_core` | `Clock<Tag>`, `ClockSnapshot`, statuses, hooks (exposed by every clock domain) |
| `ptp_types` | `//score/time/ptp:ptp_types` | PTP data types (exposed by VehicleTime) |
| `high_res_steady_time` | `//score/time/high_res_steady_time:interface` | HighResSteadyTime domain |
| `steady_time` | `//score/time/steady_time:interface` | SteadyTime domain |
| `system_time` | `//score/time/system_time:interface` | SystemTime domain |
| `vehicle_time` | `//score/time/vehicle_time:interface` | VehicleTime domain |
| `time_daemon_core` | `//score/time_daemon:time_daemon_core` | `TimeDaemon`, `TimebaseHandler`, `CreateTimebaseHandlers()` |
| `time_daemon_main` | `//score/time_daemon:time_daemon_main` | Same as `time_daemon_core`, for custom daemon executables |
| `time_daemon_timebase_handler_factory` | `//score/time_daemon:timebase_handler_factory` | `CreateTimebaseHandlers()` |
| `time_daemon_generic_timebase_handler` | `//score/time_daemon:generic_timebase_handler` | `GenericTimebaseHandler<T>` |
| `time_daemon_gptp_shm_machine` | `//score/time_daemon:gptp_shm_machine` | `CreateGPTPShmMachine()` |
| `time_daemon_msg_broker` | `//score/time_daemon:msg_broker` | `Topic` |
| `time_daemon_ptp_control_flow_divider` | `//score/time_daemon:ptp_control_flow_divider` | `CreatePtpControlFlowDivider()` |
| `time_daemon_ptp_machine` | `//score/time_daemon:ptp_machine` | `PTPMachine<PTPEngine>` |
| `time_daemon_ptp_time_info` | `//score/time_daemon:ptp_time_info` | `PtpTimeInfo` |
| `time_daemon_svt_publisher` | `//score/time_daemon:svt_publisher` | `CreateSvtPublisher()` |
| `time_daemon_svt_verification_machine` | `//score/time_daemon:svt_verification_machine` | `CreateSvtVerificationMachine()` |

A test covers the public headers of the target and of its dependencies in the
same Bazel package or its subpackages. Headers from other packages are not
covered. The clock domain interfaces expose `Clock<Tag>` from
`//score/time/clock` and, for VehicleTime, the subscription event types
`TimeSlaveSyncData` and `PDelayMeasurementData` from `//score/time/ptp`.
These two packages are therefore locked separately as `clock` and `ptp_types`,
although their Bazel targets are not public. Symbols in `detail`, `details`,
`internal` or `impl` namespaces are excluded by the tool. Some aliases share
headers, so one API change can update more than one lock file.

Public targets that are not covered:

- `//score/time/<domain>:<domain>` production aliases: they add only backend
  implementations to `<domain>:interface`, and the HighResSteadyTime backend
  differs between QNX and Linux.
- `//score/time/<domain>:<domain>_mock` test aliases: the mocked backend API
  is locked through `<domain>:interface`.
- Binaries: `//score/time_daemon:time_daemon` and `//score/time_slave:time_slave`.
  TimeSlave has no public C++ library.
- Non-C++ interfaces: IPC and shared-memory data layouts, the TimeSlave
  configuration schema, and documentation completeness (`check_docs = False`).

When you add a public C++ library target, add it to `_API_SURFACES` in
[`BUILD`](BUILD) and generate its lock file.

## Running the check

```bash
bazel test --config x86_64-linux //:api_surface_tests
```

`//:api_surface_tests` is a `test_suite` in the root [`BUILD`](../../BUILD) that
runs all tests of this package. A single surface, for example VehicleTime:

```bash
bazel test --config x86_64-linux //tools/api_surface:vehicle_time
```

CI runs the suite in the `api-surface` job of
[`build-linux.yml`](../../.github/workflows/build-linux.yml). A mismatch fails the
pull request. CI never updates the lock files.

## Updating the lock files after an intentional API change

1. Make sure the API change is intended and agreed. For a breaking change, check
   the impact on consumers and the release notes.
2. Regenerate the affected lock file, for example for VehicleTime:

   ```bash
   bazel run --config x86_64-linux //tools/api_surface:vehicle_time.update
   ```

   To regenerate all lock files:

   ```bash
   bazel query 'filter("\.update$", //tools/api_surface:all)' \
     | xargs -n1 bazel run --config x86_64-linux
   ```

3. Review `git diff tools/api_surface/` and check that it contains only the
   intended changes.
4. Commit the lock files together with the API change, so reviewers see both in
   the same pull request.

An updated lock file only records the new API. It does not show that the change
is backward compatible.

## Limitations

- This check finds changes to declarations. It does not check ABI or behaviour.
  Changes to class layout, inline function bodies, default member values or
  constant values can go undetected.
- Headers are parsed once, on Linux, with the hermetic LLVM toolchain
  (`@llvm_toolchain`), not with the GCC or QNX toolchains. Declarations that
  exist only under other platform conditions are not covered.
- The tests are Linux-only. They are not part of `//score/...` and do not run in
  the QNX or arm64 builds.
