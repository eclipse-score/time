# CodeQL Static Analysis — MISRA C++:2023

This directory provides a self-contained Bazel target for running MISRA C++:2023
static analysis using [CodeQL](https://codeql.github.com/) and the
[codeql-coding-standards](https://github.com/github/codeql-coding-standards)
query pack.

---

## How it works

```
bazel run //tools/static_analysis:codeql_lint -- \
    --output-dir /tmp/codeql-results \
    --output-prefix codeql-time \
    --target //score/...
```

The `codeql_lint` target orchestrates three phases:

### Phase 1 — Database creation (build tracing)

1. `codeql database init --begin-tracing` — initialises an empty database and
   writes a `start-tracing.json` environment file containing LD_PRELOAD hooks.
2. `bazel run @codeql_coding_standards//:process_coding_standards_config` —
   validates the `coding-standards.yaml` deviation records at the workspace root.
3. `bazel build --config=codeql //score/...` — rebuilds all targets with CodeQL
   env vars injected; the LD_PRELOAD tracer intercepts every compiler call and
   writes TRAP files.
4. `codeql database finalize` — merges TRAP files into the final database.

### Phase 2 — Query analysis

`codeql database analyze` runs all 218 rules from the pre-compiled
`misra-cpp-coding-standards` pack against the database and produces:

| Output | Format |
| ------ | ------ |
| `<prefix>.sarif` | SARIF v2.1 — open in VS Code with the [SARIF Viewer extension](https://marketplace.visualstudio.com/items?itemName=MS-SarifVSCode.sarif-viewer) |
| `<prefix>.csv`   | Flat findings table: rule, file, line, message |

### Phase 3 — Compliance reports (`analysis_reports/`)

`analysis_report` (from `@codeql_coding_standards`) generates four Markdown
reports from the database + SARIF:

| File | Contents |
| ---- | -------- |
| `database_integrity_report.md` | Extraction errors per source file |
| `deviations_report.md`         | Active deviations from `coding-standards.yaml` |
| `guideline_compliance_summary.md` | Per-rule pass/fail summary |
| `guideline_recategorizations_report.md` | Rule recategorization log |

---

## Pinned versions

| Component | Version | Role |
| --------- | ------- | ---- |
| CodeQL CLI (`@codeql_bundle`) | 2.21.4 | Build tracer + query runner |
| codeql-coding-standards (`@codeql_coding_standards`) | 2.61.0 | Report scripts, deviation query sources |
| MISRA C++ pack (`@codeql_coding_standards_compiled`) | 2.61.0 pre-compiled | Query pack (.qlx) + bundled library packs |

All three are downloaded by Bazel from GitHub on first use (~1 GB total) and
cached in the Bazel repository cache. Subsequent runs are fully offline.

---

## Deviations

Project-level MISRA C++:2023 deviations are declared in `coding-standards.yaml`
at the repository root. Each entry requires a `rule-id`, `query-id`,
`justification`, and the affected `paths`.

---

## Known gaps and limitations

### 1. Toolchain: LLVM required, not GCC

The `--config=codeql` build uses `@llvm_toolchain` (Clang) instead of the
normal GCC 12.2.0 toolchain. This is a **hard requirement**: the CodeQL tracer
intercepts compiler calls via LD_PRELOAD by matching the binary name pattern
`^.*cc.*$` / `^.*clang.*$`. In practice, the hermetic GCC toolchain wrapper
(`cc_wrapper.sh`) is executed in a way that the tracer does not intercept
(confirmed: GCC builds produce near-empty databases with ~600 B of relations).
Clang's wrapper (`cc_wrapper.sh` from `@llvm_toolchain`) is correctly traced.

### 2. Extraction errors in external dependencies

The `database_integrity_report.md` lists ~90 extraction errors. These are all
from external repositories (`googletest`, `google_benchmark`, `score_baselibs`,
`score_lifecycle_health`). This is expected: the external sources are compiled
with flags or language extensions that CodeQL's extractor does not fully support.
Findings in `score/**` are not affected.

### 3. QNX targets not scanned

The `--config=codeql` config targets x86_64-linux. QNX targets
(`time-x86_64-qnx`, `time-arm64-qnx`) are not included in the default scan.
CodeQL does support `qcc`/`q++` compilers (explicit handler in `tracing-config.lua`),
but tracing a QCC build requires a QNX-capable host and is not set up here.

### 4. `coding-standards.yaml` is a runtime dependency, not a Bazel data dep

`process_coding_standards_config` reads `coding-standards.yaml` by scanning
`BUILD_WORKING_DIRECTORY` (the workspace root). The file is not declared as a
Bazel `data` dependency of `codeql_lint` — this is intentional (same pattern as
`eclipse-score/communication`) but means Bazel's sandbox cannot verify the file
is present before the run starts. If the file is missing the tool will proceed
with no deviations.

### 5. First run writes to `~/.codeql/packages/`

On first use, `codeql_lint.py` copies the library packs bundled inside the
compiled MISRA pack (`.codeql/libraries/`) into the CodeQL global package cache
(`~/.codeql/packages/`). This is required so that `analysis_report`'s internal
`codeql database run-queries` calls can resolve `import codingstandards.*`
without network access. Packs are only copied if not already present (idempotent).
On CI (ephemeral runners) this happens on every run; locally it happens once.
