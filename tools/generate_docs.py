#!/usr/bin/env python3
# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

"""
Auto-generate documentation (.rst and .puml files) from C++ source code.

This script scans the SCORE Time Synchronization codebase and generates:
1. API documentation in ReStructuredText (.rst) format
2. Architecture diagrams in PlantUML (.puml) format
3. Sequence diagrams for key workflows
"""

import argparse
import re
import sys
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass


@dataclass
class CppClass:
    """Represents a C++ class extracted from source."""

    name: str
    namespace: str
    brief: str
    file_path: str
    is_struct: bool = False
    members: List[str] = None
    methods: List[str] = None

    def __post_init__(self):
        if self.members is None:
            self.members = []
        if self.methods is None:
            self.methods = []


@dataclass
class CppFunction:
    """Represents a C++ function."""

    name: str
    signature: str
    brief: str
    returns: str
    file_path: str


def extract_doxygen_comment(
    lines: List[str], start_idx: int
) -> Tuple[Optional[str], int]:
    """
    Extract Doxygen-style comment block starting at start_idx.
    Returns (comment_text, next_line_index)
    """
    if start_idx >= len(lines):
        return None, start_idx

    line = lines[start_idx].strip()

    # Check for /** ... */ style
    if line.startswith("/**"):
        comment_lines = []
        i = start_idx
        in_comment = True

        while i < len(lines) and in_comment:
            line = lines[i].strip()
            if line.startswith("/**"):
                line = line[3:].strip()
            if line.endswith("*/"):
                line = line[:-2].strip()
                in_comment = False
            elif line.startswith("*"):
                line = line[1:].strip()

            if line:
                comment_lines.append(line)
            i += 1

        return "\n".join(comment_lines), i

    # Check for /// style
    if line.startswith("///"):
        comment_lines = []
        i = start_idx
        while i < len(lines) and lines[i].strip().startswith("///"):
            line = lines[i].strip()[3:].strip()
            if line:
                comment_lines.append(line)
            i += 1
        return "\n".join(comment_lines), i

    return None, start_idx


def extract_classes_from_file(file_path: Path) -> List[CppClass]:
    """Extract C++ classes from header file."""
    classes = []

    try:
        with open(file_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return classes

    i = 0
    while i < len(lines):
        line = lines[i].strip()

        # Look for class/struct definition
        class_match = re.match(r"(class|struct)\s+(\w+)", line)
        if class_match:
            is_struct = class_match.group(1) == "struct"
            class_name = class_match.group(2)

            # Extract preceding Doxygen comment by looking backwards
            comment = extract_comment_before_line(lines, i)
            brief = (
                extract_brief_from_comment(comment)
                if comment
                else f"{class_name} class"
            )

            # Extract namespace
            namespace = extract_namespace(lines, i)

            cpp_class = CppClass(
                name=class_name,
                namespace=namespace,
                brief=brief,
                file_path=str(file_path),
                is_struct=is_struct,
            )
            classes.append(cpp_class)

        i += 1

    return classes


def extract_comment_before_line(lines: List[str], target_line: int) -> Optional[str]:
    """
    Extract Doxygen comment block immediately before target line.
    Skips empty lines and looks for /** ... */ or /// blocks.
    """
    # Look backwards from target line, skip empty lines
    i = target_line - 1
    while i >= 0 and not lines[i].strip():
        i -= 1

    if i < 0:
        return None

    # Check if we're at the end of a comment block
    line = lines[i].strip()

    # Case 1: Multi-line /** ... */ comment
    if line.endswith("*/"):
        comment_lines = []
        in_comment = True

        while i >= 0 and in_comment:
            line = lines[i].strip()

            if line.startswith("/**"):
                # Remove /** prefix
                line = line[3:].strip()
                if line.endswith("*/"):
                    line = line[:-2].strip()
                if line:
                    comment_lines.insert(0, line)
                in_comment = False
            elif line.endswith("*/"):
                # Remove */ suffix
                line = line[:-2].strip()
                if line.startswith("*"):
                    line = line[1:].strip()
                if line:
                    comment_lines.insert(0, line)
            elif line.startswith("*"):
                # Remove leading *
                line = line[1:].strip()
                if line:
                    comment_lines.insert(0, line)
            else:
                in_comment = False

            i -= 1

        return "\n".join(comment_lines) if comment_lines else None

    # Case 2: /// style comments
    if line.startswith("///"):
        comment_lines = []
        while i >= 0 and lines[i].strip().startswith("///"):
            line = lines[i].strip()[3:].strip()
            if line:
                comment_lines.insert(0, line)
            i -= 1
        return "\n".join(comment_lines) if comment_lines else None

    return None


def extract_namespace(lines: List[str], class_line_idx: int) -> str:
    """Extract namespace for a class definition."""
    namespace_parts = []
    for i in range(class_line_idx - 1, -1, -1):
        line = lines[i].strip()
        ns_match = re.match(r"namespace\s+([\w:]+)", line)
        if ns_match:
            namespace_parts.insert(0, ns_match.group(1))
    return "::".join(namespace_parts) if namespace_parts else ""


def extract_brief_from_comment(comment: str) -> str:
    """Extract @brief description from Doxygen comment."""
    if not comment:
        return ""

    # Try to extract @brief tag
    brief_match = re.search(r"@brief\s+(.+?)(?:\n\n|\n@|$)", comment, re.DOTALL)
    if brief_match:
        return brief_match.group(1).strip().replace("\n", " ")

    # If no @brief tag, use first paragraph
    lines = comment.split("\n")
    brief_lines = []
    for line in lines:
        line = line.strip()
        if not line:  # Empty line ends the brief
            break
        if line.startswith("@"):  # Tag starts, end brief
            break
        brief_lines.append(line)

    return " ".join(brief_lines) if brief_lines else ""


def extract_full_description(comment: str) -> str:
    """Extract full description from Doxygen comment, including examples."""
    if not comment:
        return ""

    # Remove @brief section
    desc = re.sub(r"@brief\s+.+?(?:\n\n|\n@)", "", comment, flags=re.DOTALL)

    # Clean up remaining text
    lines = []
    for line in desc.split("\n"):
        line = line.strip()
        if line:
            lines.append(line)

    return "\n\n   ".join(lines) if lines else ""


def generate_api_rst(component_name: str, classes: List[CppClass], output_file: Path):
    """Generate RST file for component API documentation."""
    content = f""".. ******************************************************************************
   Copyright (c) 2025 Contributors to the Eclipse Foundation

   See the NOTICE file(s) distributed with this work for additional
   information regarding copyright ownership.

   This program and the accompanying materials are made available under the
   terms of the Apache License Version 2.0 which is available at
   https://www.apache.org/licenses/LICENSE-2.0

   SPDX-License-Identifier: Apache-2.0
   ******************************************************************************

{component_name} API Reference
{"=" * (len(component_name) + 14)}

.. contents::
   :local:
   :depth: 2

Overview
--------

This module provides {component_name.lower()} for the SCORE Time Synchronization system.

"""

    # Group classes by namespace
    by_namespace = {}
    for cls in classes:
        ns = cls.namespace or "global"
        if ns not in by_namespace:
            by_namespace[ns] = []
        by_namespace[ns].append(cls)

    # Generate documentation for each namespace
    for namespace, ns_classes in sorted(by_namespace.items()):
        if namespace != "global":
            title = f"Namespace: ``{namespace}``"
            content += f"\n{title}\n"
            content += "-" * len(title) + "\n\n"

        for cls in ns_classes:
            full_name = (
                f"{namespace}::{cls.name}" if namespace != "global" else cls.name
            )

            content += f"\n{cls.name}\n"
            content += "~" * len(cls.name) + "\n\n"

            cls_type = "struct" if cls.is_struct else "class"
            content += f".. cpp:{cls_type}:: {full_name}\n\n"

            if cls.brief:
                # Wrap brief description with proper indentation
                brief_lines = cls.brief.split("\n")
                for line in brief_lines:
                    if line.strip():
                        content += f"   {line}\n"
                content += "\n"

            content += f"   **Defined in:** :code:`{Path(cls.file_path).name}`\n\n"

            # Add usage note for key classes
            if "Lock" in cls.name:
                content += (
                    "   **Usage:** RAII wrapper for automatic resource management.\n\n"
                )
            elif "Shared" in cls.name:
                content += (
                    "   **Usage:** Shared memory structure for lock-free IPC.\n\n"
                )
            elif "Parse" in cls.name:
                content += (
                    "   **Usage:** Exception-free string parsing for configuration.\n\n"
                )

    # Add examples section
    content += "\nExamples\n"
    content += "--------\n\n"
    content += "See the test files for usage examples:\n\n"
    content += "- Unit tests: ``tests/cpp/``\n"
    content += "- Integration tests: ``tests/integration/``\n\n"

    # Write to file
    output_file.parent.mkdir(parents=True, exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"✅ Generated {output_file}")


def generate_architecture_puml(output_file: Path):
    """Generate high-level architecture diagram."""
    content = """@startuml
!theme plain
skinparam componentStyle rectangle

package "SCORE Time Synchronization" {
    component [tsyncd Daemon] as daemon
    component [libscore_time] as lib
    database "Shared Memory" as shm

    package "Common Utilities" {
        component [ClockNs] as clock
        component [PthreadLockGuard] as lock
        component [ParseInteger/Double] as parser
    }
}

cloud "Network" {
    component [gPTP Master] as gptp
    component [NTP Server] as ntp
}

actor "Application" as app

daemon --> gptp : "Sync via\\nEthernet/PTP"
daemon --> ntp : "Query via\\nUDP/123"
daemon --> shm : "Writes time\\n(seqlock)"
lib --> shm : "Reads time\\n(seqlock)"
app --> lib : "GetVehicleTime()\\nGetAbsoluteTime()"
daemon ..> clock : uses
daemon ..> lock : uses
daemon ..> parser : uses
lib ..> clock : uses

note right of shm
  Lock-free IPC
  Seqlock pattern
  Atomic operations
  Version: 4
end note

@enduml
"""

    output_file.parent.mkdir(parents=True, exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"✅ Generated {output_file}")


def generate_class_diagram_puml(output_file: Path):
    """Generate core class relationships diagram."""
    content = """@startuml
!theme plain

package "score_time::ipc" {
    class SharedState {
        +kMagic: uint32_t {static}
        +kVersion: uint16_t {static}
        --
        +magic: atomic<uint32_t>
        +version: atomic<uint16_t>
        +vehicle_seq: atomic<uint64_t>
        +vehicle_base_ns: atomic<int64_t>
        +vehicle_base_mono_ns: atomic<int64_t>
        +vehicle_acc: atomic<AccuracyQualifier>
        +vehicle_tpq: atomic<TimePointQualifier>
        --
        +vehicle_log[256]: SyncLogEntry
        +abs_log[256]: SyncLogEntry
    }

    class SyncLogEntry {
        +seq: atomic<uint64_t>
        +monotonic_ns: atomic<int64_t>
        +type: atomic<uint16_t>
        +flags: atomic<uint16_t>
        +v1: atomic<int64_t>
        +v2: atomic<int64_t>
    }

    class ShmRegion {
        -fd_: int
        -addr_: void*
        -size_: size_t
        --
        +Open(name): bool
        +Close(): void
        +Addr(): void*
        +Size(): size_t
    }

    SharedState *-- "512" SyncLogEntry : contains
    ShmRegion ..> SharedState : "maps to"
}

package "score_time::utils" {
    class PthreadLockGuard {
        -mtx_: pthread_mutex_t*
        --
        +PthreadLockGuard(mtx)
        +~PthreadLockGuard()
        --
        {method} Deleted: copy, move
    }

    class "ParseInteger<T>" as ParseInt {
        +ParseInteger(str, out): bool {static}
    }

    class ParseDouble {
        +ParseDouble(str, out): bool {static}
    }

    class ClockNs {
        +ClockNsSafe(clk): optional<int64_t> {static}
        +ClockNs(clk): int64_t {static}
    }
}

package "tsyncd" {
    class TimeSyncEngine {
        -ctx_: Context
        -shared_: SharedState*
        -shm_: ShmRegion
        --
        +Start(): bool
        +Stop(): void
        -RxLoop(): void
        -PdelayLoop(): void
        -AbsLoop(): void
    }
}

TimeSyncEngine --> SharedState : writes to
TimeSyncEngine --> ShmRegion : owns
TimeSyncEngine ..> PthreadLockGuard : uses
TimeSyncEngine ..> ClockNs : uses

note right of SharedState
  Seqlock protocol:
  seq odd = writing
  seq even = readable
end note

note right of PthreadLockGuard
  RAII lock wrapper
  Aborts on error
end note

@enduml
"""

    output_file.parent.mkdir(parents=True, exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"✅ Generated {output_file}")


def generate_sequence_gptp_puml(output_file: Path):
    """Generate gPTP synchronization sequence diagram."""
    content = """@startuml
!theme plain

participant "gPTP Master" as Master
participant "tsyncd::RxLoop" as RxLoop
participant "SharedState" as SHM
participant "Client App" as Client

== gPTP Synchronization ==

Master -> RxLoop: Sync message\\n(HW timestamp t1)
activate RxLoop
RxLoop -> RxLoop: Extract timestamp
RxLoop -> Master: Delay_Req\\n(HW timestamp t3)
deactivate RxLoop

Master -> RxLoop: Delay_Resp\\n(t2, t4 timestamps)
activate RxLoop

RxLoop -> RxLoop: Calculate offset\\noffset = ((t2-t1) + (t3-t4)) / 2

RxLoop -> SHM: WriteVehicle(base_ns, base_mono_ns, ...)
note right
  Seqlock write protocol:
  1. seq.fetch_add(1, release) // odd = writing
  2. Write all fields with release stores
  3. seq.fetch_add(1, release) // even = complete
end note
deactivate RxLoop

...Time passes...

Client -> SHM: ReadVehicle(out_base_ns, out_base_mono_ns, ...)
activate Client
note right
  Seqlock read protocol (retry up to 1000x):
  1. seq_a = seq.load(acquire)
  2. if seq_a is odd: retry
  3. Read all fields with acquire loads
  4. seq_b = seq.load(acquire)
  5. if seq_a != seq_b: retry
  6. Success!
end note
SHM --> Client: vehicle_base_ns, accuracy
Client -> Client: current = base_ns + (now_mono - base_mono)
deactivate Client

@enduml
"""

    output_file.parent.mkdir(parents=True, exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"✅ Generated {output_file}")


def update_index_rst(docs_dir: Path, has_api: bool, has_diagrams: bool):
    """Update docs/index.rst to include generated documentation."""
    index_file = docs_dir / "index.rst"

    # Read existing content
    if index_file.exists():
        with open(index_file, "r", encoding="utf-8") as f:
            content = f.read()
    else:
        content = ""

    # Check if toctree exists
    if ".. toctree::" not in content:
        # Append new toctree
        additions = "\n\n"
        if has_api:
            additions += "API Reference\n"
            additions += "-------------\n\n"
            additions += ".. toctree::\n"
            additions += "   :maxdepth: 2\n\n"
            additions += "   api/index\n\n"

        if has_diagrams:
            additions += "Architecture Diagrams\n"
            additions += "--------------------\n\n"
            additions += ".. toctree::\n"
            additions += "   :maxdepth: 1\n\n"
            additions += "   diagrams/index\n\n"

        with open(index_file, "a", encoding="utf-8") as f:
            f.write(additions)

        print(f"✅ Updated {index_file}")


def main():
    parser = argparse.ArgumentParser(description="Generate SCORE Time documentation")
    parser.add_argument("--api", action="store_true", help="Generate API documentation")
    parser.add_argument(
        "--arch", action="store_true", help="Generate architecture diagrams"
    )
    parser.add_argument("--seq", action="store_true", help="Generate sequence diagrams")
    parser.add_argument("--all", action="store_true", help="Generate all documentation")
    parser.add_argument(
        "--update-index", action="store_true", help="Update docs/index.rst"
    )

    args = parser.parse_args()

    # Default to --all if no options specified
    if not (args.api or args.arch or args.seq or args.all):
        args.all = True

    if args.all:
        args.api = args.arch = args.seq = args.update_index = True

    # Determine project root
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent
    docs_dir = project_root / "docs"

    print(f"Generating documentation for SCORE Time Synchronization...")
    print(f"Project root: {project_root}")

    # Generate API documentation
    if args.api:
        print("\n📚 Generating API documentation...")

        # Scan header files
        include_dirs = [
            project_root / "src" / "common" / "include" / "score_time",
            project_root / "src" / "tsyncd" / "engine",
        ]

        components = {
            "Core Types": [],
            "IPC": [],
            "Utilities": [],
        }

        for include_dir in include_dirs:
            if not include_dir.exists():
                continue

            for header_file in include_dir.rglob("*.hpp"):
                classes = extract_classes_from_file(header_file)

                # Categorize classes
                file_str = str(header_file)
                if "shared_state" in file_str or "types" in file_str:
                    components["Core Types"].extend(classes)
                elif "ipc" in file_str or "shm" in file_str:
                    components["IPC"].extend(classes)
                elif "utils" in file_str:
                    components["Utilities"].extend(classes)

        # Generate RST files
        api_dir = docs_dir / "api"
        for component_name, classes in components.items():
            if classes:
                output_file = (
                    api_dir / f"{component_name.lower().replace(' ', '_')}.rst"
                )
                generate_api_rst(component_name, classes, output_file)

        # Generate API index
        api_index = api_dir / "index.rst"
        api_index_content = """API Reference
=============

.. toctree::
   :maxdepth: 2

"""
        for component_name in components.keys():
            filename = component_name.lower().replace(" ", "_")
            api_index_content += f"   {filename}\n"

        api_dir.mkdir(parents=True, exist_ok=True)
        with open(api_index, "w", encoding="utf-8") as f:
            f.write(api_index_content)
        print(f"✅ Generated {api_index}")

    # Generate architecture diagrams
    if args.arch:
        print("\n🏗️  Generating architecture diagrams...")
        diagrams_dir = docs_dir / "diagrams"

        generate_architecture_puml(diagrams_dir / "architecture.puml")
        generate_class_diagram_puml(diagrams_dir / "class_relationships.puml")

    # Generate sequence diagrams
    if args.seq:
        print("\n📊 Generating sequence diagrams...")
        diagrams_dir = docs_dir / "diagrams"

        generate_sequence_gptp_puml(diagrams_dir / "sequence_gptp_sync.puml")

        # Generate diagrams index
        diagrams_index = diagrams_dir / "index.rst"
        diagrams_index_content = """Architecture Diagrams
=====================

System Architecture
-------------------

.. uml:: architecture.puml
   :caption: High-level system architecture

Class Relationships
-------------------

.. uml:: class_relationships.puml
   :caption: Core class relationships

Sequence Diagrams
-----------------

gPTP Synchronization
~~~~~~~~~~~~~~~~~~~~

.. uml:: sequence_gptp_sync.puml
   :caption: gPTP synchronization workflow
"""

        with open(diagrams_index, "w", encoding="utf-8") as f:
            f.write(diagrams_index_content)
        print(f"✅ Generated {diagrams_index}")

    # Update index
    if args.update_index:
        update_index_rst(docs_dir, args.api, args.arch or args.seq)

    print("\n✅ Documentation generation complete!")
    print(f"\nNext steps:")
    print(f"  1. Review generated files in {docs_dir}")
    print(f"  2. Build documentation: bazel run //:docs")
    print(f"  3. View HTML: open bazel-bin/docs/docs/html/index.html")


if __name__ == "__main__":
    main()
