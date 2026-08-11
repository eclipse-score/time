# Time

Unified time service library and distributed time synchronization infrastructure for automotive ECU software.

[![Documentation](https://img.shields.io/badge/docs-time-blue)](https://eclipse-score.github.io/time)

## Overview

Portable and high-performance implementation of time services for the S-CORE project.

This repository contains source code for time clocks, time distribution infrastructure, and PTP synchronization. The Time module is implemented in C++ and provides working examples demonstrating usage patterns.

High-level functionality provided by the Time module:

- **Clock Domains**: Four time sources accessed through unified `Clock<Tag>` API; all domains support nanosecond resolution and compile-time domain selection preventing cross-domain timing errors; mock backends and `ScopedClockOverride` available for testing
  - **SystemTime**: Wall-clock time (Unix epoch) for timestamps and user-visible time displays
  - **SteadyTime**: Monotonic time for duration measurements and timeouts
  - **HighResSteadyTime**: High-resolution monotonic time for precise timing applications
  - **VehicleTime**: PTP-synchronized time for distributed automotive applications requiring initialization
- **Clock API**: Type-safe `Clock<Tag>::GetInstance().Now()` returns time and status information
- **Event Subscription**: `Subscribe<EventType>()` / `Unsubscribe<EventType>()` for status changes and PTP timebase data
- **Time Infrastructure**
  - **TimeDaemon**: Standalone daemon that retrieves synchronized time from TimeSlave, validates timepoints, sets synchronization status, and distributes VehicleTime to applications
  - **TimeSlave**: gPTP (IEEE 802.1AS) slave endpoint that implements network synchronization protocol, calculates time offset, and publishes synchronization time data to the TimeDaemon

---

## 📋 Public API

### Clock Domains

#### SystemTime

| Target | Purpose |
|--------|---------|
| `//score/time/system_time` | Wall-clock time (Unix epoch) for timestamps |
| `//score/time/system_time:system_time_mock` | Mock backend for SystemTime testing |
| `//score/time/system_time:interface` | Header-only interface (no backend) |

#### SteadyTime

| Target | Purpose |
|--------|---------|
| `//score/time/steady_time` | Monotonic time for duration measurement |
| `//score/time/steady_time:steady_time_mock` | Mock backend for SteadyTime testing |
| `//score/time/steady_time:interface` | Header-only interface (no backend) |

#### HighResSteadyTime

| Target | Purpose |
|--------|---------|
| `//score/time/high_res_steady_time` | High-resolution monotonic time |
| `//score/time/high_res_steady_time:high_res_steady_time_mock` | Mock backend for HighResSteadyTime testing |
| `//score/time/high_res_steady_time:interface` | Header-only interface (no backend) |

#### VehicleTime

| Target | Purpose |
|--------|---------|
| `//score/time/vehicle_time` | PTP-synchronized vehicle time |
| `//score/time/vehicle_time:vehicle_time_mock` | Mock backend for VehicleTime testing |
| `//score/time/vehicle_time:interface` | Header-only interface (no backend) |

### Test Utilities

| Target | Purpose |
|--------|---------|
| `//score/time/clock:clock_test_utils` | ScopedClockOverride and ClockTestFactory utilities |

---

## ⚙️ Using as Dependency

Add to your `MODULE.bazel`:

```python
bazel_dep(name = "score_time", version = "x.x.x")
```

Check available versions in the [S-CORE Bazel Registry](https://eclipse-score.github.io/bazel_registry_ui/modules/score_time).

### Using Unreleased Versions

To depend on an unreleased version (for development or testing), use a git override in your `MODULE.bazel`:

```python
git_override(
    module_name = "score_time",
    commit = "abc123...",
    remote = "https://github.com/eclipse-score/time.git",
)
```

Replace the `commit` value with the specific git hash you want to use.

### Executable Artifacts

For deployment, use these executable targets:

Available binaries:

- `@score_time//score/time_daemon:time_daemon` - TimeDaemon executable for time distribution
- `@score_time//score/time_slave:time_slave` - TimeSlave executable for PTP synchronization

Run artifacts directly:

```bash
bazel run @score_time//score/time_daemon:time_daemon
bazel run @score_time//score/time_slave:time_slave
```

Do not add these to `deps` as libraries. Use them as runtime artifacts for your deployment system.

---

## 📖 Documentation

- **[Time Feature Documentation](https://eclipse-score.github.io/score/main/features/time/index.html)**: High-level feature overview and S-CORE platform integration
- **[Time Module Documentation](https://eclipse-score.github.io/time)**: Detailed API documentation, architecture, and requirements

Generate module documentation locally:

```bash
bazel run //:docs
```

---

## 🚀 Getting Started

This section contains information on how to build and use the Time module.

### Clone the Repository

```bash
git clone https://github.com/eclipse-score/time.git
cd time
```

### Prerequisites

- **C++ Compiler**: gcc/clang with C++17 support
- **Build System**: Bazel 8+ (managed via Bazelisk)
- **Operating System**: Linux (Ubuntu 24.04+)
- **Dependencies**: S-CORE Baselibs, Google Test
- **For QNX targets**: QNX 8.0 SDP

### Development Environment

Use devcontainer as default path. It includes Bazel tooling and dependencies.

1. Install Docker.
2. Open repository in a devcontainer-capable editor (for example VS Code with Dev Containers extension).
3. Reopen workspace in container.

Follow the [S-CORE Development Environment Guide](https://eclipse-score.github.io/score/main/contribute/development/development_environment.html) for Linux host setup requirements.

### Building the Project

Build all components for **Linux x86_64** by running:

```bash
bazel build --config=time-x86_64-linux //score/... //examples/...
```

Run all tests:

```bash
bazel test --config=time-x86_64-linux //score/... //examples/...
```

#### Other Platforms

**Linux AArch64**:
```bash
bazel build --config=time-arm64-linux //score/... //examples/...
bazel test --config=time-arm64-linux //score/... //examples/...
```

**QNX x86_64**:
```bash
bazel build --config=time-x86_64-qnx //score/... //examples/...
bazel test --config=time-x86_64-qnx //score/... //examples/...
```

**QNX AArch64**:
```bash
bazel build --config=time-aarch64-qnx //score/... //examples/...
bazel test --config=time-aarch64-qnx //score/... //examples/...
```

#### Testing with Sanitizers

To test with AddressSanitizer, UBSan, and LeakSanitizer enabled:

```bash
bazel test --config=time-x86_64-linux --config=asan_ubsan_lsan --build_tests_only //score/... //examples/...
```

---

## 🛠 Tools & Linters

### Clang-tidy

```sh
bazel test --config=clang-tidy //score/...
```

### CodeQL — MISRA C++:2023 static analysis

Runs a full build-traced MISRA C++:2023 scan using the vendored CodeQL CLI and
pre-compiled query pack. The first run downloads ~1 GB of tooling (CodeQL CLI +
MISRA pack) into the Bazel repository cache; subsequent runs reuse the cache.

```sh
bazel run //tools/static_analysis:codeql_lint -- \
    --output-dir /tmp/codeql-results \
    --output-prefix codeql-time \
    --target //score/...
```

**Results** are written to `--output-dir` on completion:

| File | Contents |
| ---- | -------- |
| `codeql-time.sarif` | SARIF v2.1 — import into VS Code ([SARIF Viewer extension](https://marketplace.visualstudio.com/items?itemName=MS-SarifVSCode.sarif-viewer)) or any SARIF-aware tool |
| `codeql-time.csv` | Flat findings table: rule, file, line, message |
| `analysis_reports/database_integrity_report.md` | Extraction errors (external deps are expected) |
| `analysis_reports/deviations_report.md` | Active MISRA C++:2023 deviations |
| `analysis_reports/guideline_compliance_summary.md` | Per-rule compliance summary |
| `analysis_reports/guideline_recategorizations_report.md` | Rule recategorization log |

To open SARIF results in VS Code: install the **SARIF Viewer** extension, then
`Ctrl+Shift+P` → *SARIF: Open SARIF file* → select `codeql-time.sarif`.

---

## 💡 Examples

Working examples demonstrating clock usage patterns, testing approaches, and integration techniques are available in the [examples/](examples/) directory:

- **examples/time/system_time** — SystemTime usage for wall-clock timestamps
- **examples/time/steady_time** — SteadyTime usage for duration measurements
- **examples/time/high_res_steady_time** — HighResSteadyTime usage for high-resolution timing
- **examples/time/vehicle_time** — VehicleTime usage with PTP synchronization

Each example includes a handler demonstrating the Clock API and corresponding unit tests.

---

## 📂 Repository Structure

```
├── score/
│   ├── time/                    # Clock domains (SystemTime, SteadyTime, etc.)
│   ├── time_daemon/             # Time distribution daemon
│   ├── time_slave/              # PTP timebase implementation
│   └── ts_client/               # Library for time slave communication
├── examples/                    # Usage examples and patterns
├── docs/                        # Module documentation
└── tools/                       # Build and development utilities
```

---

## 🤝 Contributing

See our [Contributing Guide](CONTRIBUTION.md) for contribution guidelines and development workflow.

---

## 🔗 Support

### Community

- **Issues**: Report bugs and request features via [GitHub Issues](https://github.com/eclipse-score/time/issues)
- **Discussions**: Join time [Slack Channel](https://sdvworkinggroup.slack.com/archives/C0ADTA4SKUH)
