#!/usr/bin/env python3
# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
"""Check per-component line and branch coverage from an LLVM coverage report.

Reads lcov_report/lcov.dat from the unpacked reporter output directory,
groups every tracked source file by its score/<component>/ prefix, computes
line and branch coverage per component, and exits non-zero if any component
falls below either threshold.

Components are discovered automatically from the report — no hardcoded list.
A component is the path segment directly below score/ (e.g. score/time/).

Usage:
    python3 check_coverage.py --coverage-dir DIR [--min-line PCT] [--min-branch PCT]

Arguments:
    --coverage-dir    Directory produced by unzipping the reporter output zip.
    --min-line        Minimum line coverage %% per component (default: 85).
    --min-branch      Minimum branch coverage %% per component (default: 0,
                      i.e. not enforced unless explicitly set).

Exit codes:
    0  All components meet both thresholds.
    1  One or more components are below a threshold.
    2  Coverage report not found.
"""

import argparse
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--coverage-dir", type=Path, required=True, metavar="DIR",
                   help="directory produced by unzipping the reporter output zip")
    p.add_argument("--min-line", type=float, default=85.0, metavar="PCT",
                   help="minimum effective line coverage %% per component (default: 85)")
    p.add_argument("--min-branch", type=float, default=0.0, metavar="PCT",
                   help="minimum effective branch coverage %% per component (default: 0 = not enforced)")
    return p.parse_args()


def component_of(path: str) -> str | None:
    """Return the score/<component>/ prefix for a workspace source path, or None.

    Skips files from external repositories (path contains /external/).
    """
    if "/external/" in path or "bazel-out/" in path:
        return None
    marker = "/score/"
    idx = path.find(marker)
    if idx == -1:
        return None
    rest = path[idx + len(marker):]
    parts = rest.split("/")
    return f"score/{parts[0]}/" if parts[0] else None


def parse_lcov(lcov_path: Path) -> dict[str, tuple[int, int, int, int]]:
    """Return {component: (lines_hit, lines_found, branches_hit, branches_found)}."""
    totals: dict[str, list[int]] = {}
    current: str | None = None
    for line in lcov_path.read_text(errors="replace").splitlines():
        if line.startswith("SF:"):
            current = component_of(line[3:])
        elif current:
            if line.startswith("LH:"):
                totals.setdefault(current, [0, 0, 0, 0])[0] += int(line[3:])
            elif line.startswith("LF:"):
                totals.setdefault(current, [0, 0, 0, 0])[1] += int(line[3:])
            elif line.startswith("BRH:"):
                totals.setdefault(current, [0, 0, 0, 0])[2] += int(line[4:])
            elif line.startswith("BRF:"):
                totals.setdefault(current, [0, 0, 0, 0])[3] += int(line[4:])
    return {k: tuple(v) for k, v in sorted(totals.items())}  # type: ignore[return-value]


def main() -> None:
    args = parse_args()
    lcov_path = args.coverage_dir / "lcov_report" / "lcov.dat"
    if not lcov_path.exists():
        print(f"::warning::Coverage report not found: {lcov_path}")
        sys.exit(2)

    totals = parse_lcov(lcov_path)
    line_threshold = args.min_line
    branch_threshold = args.min_branch
    check_branches = branch_threshold > 0

    header = f"{'Component':<32} {'Line%':>8}  {'Branch%':>8}  (line≥{line_threshold:.0f}%"
    header += f", branch≥{branch_threshold:.0f}%)" if check_branches else ")"
    print(f"\n{header}")
    print("-" * (len(header) + 4))

    failed: list[str] = []
    for comp, (lh, lf, brh, brf) in totals.items():
        line_pct = lh / lf * 100 if lf else 0.0
        branch_pct = brh / brf * 100 if brf else 0.0

        line_ok = line_pct >= line_threshold
        branch_ok = (not check_branches) or branch_pct >= branch_threshold
        status = "✓" if (line_ok and branch_ok) else "✗ FAIL"

        branch_str = f"  {branch_pct:>7.1f}%" if check_branches else ""
        suffix = f" (line)" if not line_ok else (" (branch)" if not branch_ok else "")
        print(f"{comp:<32} {line_pct:>7.1f}%{branch_str}  {status}{suffix}")

        if not line_ok:
            failed.append(f"{comp}: line {line_pct:.1f}% < {line_threshold:.0f}%")
        if not branch_ok:
            failed.append(f"{comp}: branch {branch_pct:.1f}% < {branch_threshold:.0f}%")

    print()
    if failed:
        for msg in failed:
            print(f"::error::Coverage gate failed — {msg}")
        sys.exit(1)
    print("All components meet the thresholds.")


if __name__ == "__main__":
    main()
