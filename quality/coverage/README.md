# Coverage Pipeline

This directory contains the LLVM source-based coverage pipeline for `score/time`,
`score/time_daemon`, and `score/time_slave`.

---

## Why LLVM Source-Based Coverage

GCC/gcov instruments at assembly level and generates **phantom branches** for:

- Exception-unwind edges on every non-`noexcept` call site
- GMock internal bookkeeping branches (all mock files)
- Assertion-abort edges only coverable via death-test subprocesses

These phantom branches permanently force mock files and PTP headers to ~50%
branch coverage regardless of test quality, making the 90% branch goal
unreachable with gcov.

LLVM source-based coverage (`llvm-cov`) tracks **source regions**, not assembly
branches. The result: no phantom branches, accurate metrics, and a 95%+ line
coverage baseline with gaps only from genuinely untested code paths.

---

## File Layout

```
quality/coverage/
├── BUILD                    # coverage_scope target (production file allowlist)
├── coverage.bazelrc         # All Bazel coverage flags — imported by .bazelrc
├── coverage_scope.bzl       # Starlark rule + aspect for allowlist generation
├── check_coverage.py        # Per-component threshold gating script
└── llvm_cov/
    ├── BUILD                # merger, reporter, reporter_wrapper targets
    ├── merger.py            # Per-test profraw → profdata zip generator
    ├── reporter.py          # Final HTML + LCOV report generator
    └── reporter_wrapper.bzl # Bakes allowlist + baseline-objects into the launcher
```

---

## Architecture: Coverage Scope

The **`coverage_scope` rule** uses a Bazel aspect to traverse the `deps` graph of
declared production library targets and emit two generated files:

| File | Contents |
|---|---|
| `time_coverage_scope_allowlist.txt` | One workspace-relative source path per line |
| `time_coverage_scope_objects.txt` | Paths to `.a` archives for baseline coverage |

The **`reporter_wrapper` rule** generates a shell launcher that calls `reporter.py`
with `--coverage_allowlist` and `--baseline_objects` pre-baked. No manual
`--ignore_filename_regex` flags are needed.

### Production scope roots (`quality/coverage/BUILD`)

The scope is rooted at the package-level production library targets. The aspect
traverses their `deps` **transitively**, so adding a new sub-library as a dep of
an existing root automatically includes it.

When a new **top-level** production library is added (not reachable via any
existing root's dep chain), add it to `coverage_scope(deps=[...])` in
[`quality/coverage/BUILD`](BUILD).

### Baseline coverage

Files in the allowlist that compiled but were never exercised by any test appear
at **0% coverage** in the report rather than being silently omitted. This makes
coverage gaps visible without any test having to explicitly import the file.

---

## Pipeline Data Flow

```
bazel coverage --build_tests_only //score/...
       │
       ├─ [per test] merger.py
       │    profraw ──────────────► profdata zip
       │
       └─ [once, final] reporter_wrapper.sh
            │  calls reporter.py with:
            │    --coverage_allowlist=time_coverage_scope_allowlist.txt
            │    --baseline_objects=time_coverage_scope_objects.txt
            │    <Bazel standard coverage args>
            │
            └─ reporter.py
                 llvm-profdata merge all profdata ──► merged.profdata
                 llvm-cov export (LCOV)           ──► lcov.dat
                 llvm-cov show  (HTML)            ──► html_report/index.html
                 [filtered to allowlist; baseline objects fill 0% gaps]
                 zip output ──► _coverage_report.dat
```

---

## Running Coverage Locally

```sh
# Run all tests and generate the coverage report
bazel coverage --build_tests_only //score/...

# Unpack the report
output_path="$(bazel info output_path)"
unzip -o "${output_path}/_coverage/_coverage_report.dat" -d coverage_output/

# Open HTML report
xdg-open coverage_output/html_report/index.html

# Check per-component thresholds
python3 quality/coverage/check_coverage.py \
  --coverage-dir coverage_output/ \
  --min-line 85 \
  --min-branch 70
```

**QNX coverage** resets all LLVM flags and uses the gcov pipeline. Add
`--config=time-x86_64-qnx` to any coverage command to activate it.

---

## `check_coverage.py` — Threshold Gating

Parses `coverage_output/lcov_report/lcov.dat`, groups files by component
(`score/<component>/`), and prints a summary table:

```
Component           Lines           Branches
score/time          95.4%  (245/257) 88.2%  (90/102)
score/time_daemon   82.1%  (...)     ...
```

Exits with code 1 if any component falls below `--min-line` or `--min-branch`.
Used in CI (`code-coverage.yml`).

### Arguments

| Flag | Default | Description |
|---|---|---|
| `--coverage-dir` | required | Directory containing `lcov_report/lcov.dat` |
| `--min-line` | `85.0` | Minimum line coverage % per component |
| `--min-branch` | `0.0` | Minimum branch coverage % per component |

---

## Extending the Scope

### Adding a new clock domain

The new domain's production library (e.g. `//score/time/my_clock:my_clock`) is
automatically included if it is a transitive dep of any existing entry in
`coverage_scope(deps=[...])`. Otherwise add it explicitly:

```python
# quality/coverage/BUILD
coverage_scope(
    name = "time_coverage_scope",
    ...
    deps = [
        ...
        "//score/time/my_clock:my_clock",
    ],
)
```

### Adding a new time_daemon sub-component

Same rule: if not reachable from `svt_handler` or `ipc/svt/receiver:factory`,
add the production library target to `coverage_scope(deps=[...])`.

---

## Bazel Feature: Death-Test Coverage

`enable_llvm_coverage_for_death_tests` (defined in [`llvm_cov/BUILD`](llvm_cov/BUILD))
adds `-mllvm -runtime-counter-relocation` compiler flags. This enables
`LLVM_PROFILE_CONTINUOUS_MODE=1` so that `ASSERT_DEATH` subprocesses write
`.profraw` files before the process exits, making death-test branches coverable.

The feature is wired via `llvm.toolchain(extra_known_features=[...])` in
`MODULE.bazel` and activated automatically for all coverage builds via
`coverage --features=enable_llvm_coverage_for_death_tests` in `coverage.bazelrc`.
