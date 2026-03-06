
# C++ & Rust Bazel Template Repository

This repository serves as a **template** for setting up **C++ and Rust projects** using **Bazel**.
It provides a **standardized project structure**, ensuring best practices for:

- **Build configuration** with Bazel.
- **Testing** (unit and integration tests).
- **Documentation** setup.
- **CI/CD workflows**.
- **Development environment** configuration.

---

## 📂 Project Structure

| File/Folder                         | Description                                       |
| ----------------------------------- | ------------------------------------------------- |
| `README.md`                         | Short description & build instructions            |
| `src/`                              | Source files for the module                       |
| `tests/`                            | Unit tests (UT) and integration tests (IT)        |
| `examples/`                         | Example files used for guidance                   |
| `docs/`                             | Documentation (Doxygen for C++ / mdBook for Rust) |
| `.github/workflows/`                | CI/CD pipelines                                   |
| `.vscode/`                          | Recommended VS Code settings                      |
| `.bazelrc`, `MODULE.bazel`, `BUILD` | Bazel configuration & settings                    |
| `project_config.bzl`                | Project-specific metadata for Bazel macros        |
| `LICENSE.md`                        | Licensing information                             |
| `CONTRIBUTION.md`                   | Contribution guidelines                           |

---

## 🚀 Getting Started

### 1️⃣ Clone the Repository

```sh
git clone https://github.com/eclipse-score/YOUR_PROJECT.git
cd YOUR_PROJECT
```

### 2️⃣ Build

This project supports cross-compilation for two target platforms. Use the
`--config` flag to select the target:

| Config flag         | Target platform                  |
| ------------------- | -------------------------------- |
| `--config=arm64-linux` | AArch64 Linux (GCC 12.2.0)    |
| `--config=arm64-qnx`   | AArch64 QNX SDP 8.0.0         |

**Build for Linux AArch64:**

```sh
bazel build //src/... --config=arm64-linux
```

**Build for QNX AArch64:**

```sh
bazel build //src/... --config=arm64-qnx
```

### 3️⃣ Run Tests

```sh
bazel test //tests/...
```

---

## 🔧 Cross-Compilation Setup

### Linux AArch64

No special setup required beyond Bazel itself. The GCC 12.2.0 cross-toolchain
is downloaded automatically by Bazel on first use.

### QNX AArch64 — Prerequisites

Building for QNX requires a valid QNX SDP 8.0 license and a registered QNX
developer account. Three items must be configured before building.

#### 1. QNX Account Credentials

The build system uses a credential helper (`scripts/internal/qnx_creds.py`) to
authenticate with `software.qnx.com` when downloading the QNX SDP toolchain.

**Option A — Environment variables (recommended for CI):**

```sh
export SCORE_QNX_USER=your@email.com
export SCORE_QNX_PASSWORD=yourpassword
```

**Option B — `~/.netrc` (recommended for local development):**

Add the following line to `~/.netrc`:

```
machine qnx.com login your@email.com password yourpassword
```

Then restrict file permissions:

```sh
chmod 600 ~/.netrc
```

#### 2. QNX License File

A valid QNX floating or node-locked license file must be present on the build
machine. The default expected path is:

```
/opt/score_qnx/license/licenses
```

This path is configured via the `license_path` parameter in `MODULE.bazel`:

```python
gcc.toolchain(
    name = "score_qcc_aarch64_qnx_toolchain",
    target_cpu = "aarch64",
    target_os = "qnx",
    use_default_package = True,
    sdp_version = "8.0.0",
    license_path = "/opt/score_qnx/license/licenses",  # <-- update this if needed
)
```

The `license_path` must point to the **license file itself**, not the directory.
Adjust this value to match the actual path on your machine.

After changing `MODULE.bazel`, run:

```sh
bazel sync --configure
```

to re-fetch the toolchain with the updated configuration.

#### 3. QNX Configuration Directory

The toolchain writes temporary license data to `/var/tmp/.qnx`. This directory
is created automatically; no manual setup is required.

---

## 🛠 Tools & Linters

The template integrates **tools and linters** from **centralized repositories** to ensure consistency across projects.

- **C++:** `clang-tidy`, `cppcheck`, `Google Test`
- **Rust:** `clippy`, `rustfmt`, `Rust Unit Tests`
- **CI/CD:** GitHub Actions for automated builds and tests

---

## 📖 Documentation

- A **centralized docs structure** is planned.

---

## ⚙️ `project_config.bzl`

This file defines project-specific metadata used by Bazel macros, such as `dash_license_checker`.

### 📌 Purpose

It provides structured configuration that helps determine behavior such as:

- Source language type (used to determine license check file format)
- Safety level or other compliance info (e.g. ASIL level)

### 📄 Example Content

```python
PROJECT_CONFIG = {
    "asil_level": "QM",  # or "ASIL-A", "ASIL-B", etc.
    "source_code": ["cpp", "rust"]  # Languages used in the module
}
```

### 🔧 Use Case

When used with macros like `dash_license_checker`, it allows dynamic selection of file types
 (e.g., `cargo`, `requirements`) based on the languages declared in `source_code`.
