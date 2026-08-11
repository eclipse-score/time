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

"""Coverage scope rule: derives file-level allowlists from cc_library dep graphs.

The aspect traverses the build graph starting from the declared `deps`, walking
any `deps`, `implementation_deps`, and `exported_deps` edges. At each cc_library
it collects the actual source files (srcs + hdrs). External files and generated
files are excluded.

The resulting allowlist contains one workspace-relative source file path per
line. The coverage reporter uses this to restrict the HTML/LCOV report to
exactly the files that are part of the declared production scope.
"""

visibility(["//..."])

_CoverageScopeInfo = provider(
    doc = "Carries source file paths and object files collected by the coverage scope aspect.",
    fields = {
        "source_files": "Depset of source file path strings (workspace-relative).",
        "object_files": "Depset of compiled .a File objects for baseline coverage.",
    },
)

def _coverage_scope_aspect_impl(target, ctx):
    """Collects source file paths and archive files from the build graph."""
    direct_files = []
    direct_archives = []
    transitive = []
    transitive_archives = []

    if CcInfo in target:
        for attr_name in ["srcs", "hdrs"]:
            if hasattr(ctx.rule.attr, attr_name):
                for src in getattr(ctx.rule.attr, attr_name):
                    for f in src.files.to_list():
                        # f.path is the exec-root path: always "external/<repo>/..."
                        # for external deps, in both WORKSPACE and bzlmod layouts.
                        # f.short_path ("../repo/...") is stored — not used for filtering.
                        if not f.path.startswith("external/") and f.is_source:
                            direct_files.append(f.short_path)

        # Collect only workspace-internal archives; @@// labels are the workspace root in bzlmod.
        if not str(target.label).startswith("@@") or str(target.label).startswith("@@//"):
            for linker_input in target[CcInfo].linking_context.linker_inputs.to_list():
                for lib in linker_input.libraries:
                    for archive in [lib.static_library, lib.pic_static_library]:
                        if archive and "/external/" not in archive.path and not archive.path.startswith("external/"):
                            direct_archives.append(archive)
                            break

    for attr_name in ["deps", "implementation_deps", "exported_deps"]:
        if hasattr(ctx.rule.attr, attr_name):
            for dep in getattr(ctx.rule.attr, attr_name):
                if _CoverageScopeInfo in dep:
                    transitive.append(dep[_CoverageScopeInfo].source_files)
                    transitive_archives.append(dep[_CoverageScopeInfo].object_files)

    return [_CoverageScopeInfo(
        source_files = depset(direct_files, transitive = transitive),
        object_files = depset(direct_archives, transitive = transitive_archives),
    )]

_coverage_scope_aspect = aspect(
    implementation = _coverage_scope_aspect_impl,
    attr_aspects = ["deps", "implementation_deps", "exported_deps"],
    doc = "Traverses cc_library dep graphs to collect implementation source files.",
)

def _coverage_scope_impl(ctx):
    """Aggregates aspect results into an allowlist file and an objects manifest."""
    all_files = {}
    all_objects = []

    for dep in ctx.attr.deps:
        if _CoverageScopeInfo in dep:
            for path in dep[_CoverageScopeInfo].source_files.to_list():
                if path:
                    all_files[path] = True
            all_objects.append(dep[_CoverageScopeInfo].object_files)

    sorted_files = sorted(all_files.keys())
    object_depset = depset(transitive = all_objects)

    output = ctx.actions.declare_file(ctx.attr.name + "_allowlist.txt")
    ctx.actions.write(
        output = output,
        content = "\n".join(sorted_files) + "\n" if sorted_files else "",
    )

    archive_paths = sorted({f.short_path: None for f in object_depset.to_list()}.keys())
    objects_output = ctx.actions.declare_file(ctx.attr.name + "_objects.txt")
    ctx.actions.write(
        output = objects_output,
        content = "\n".join(archive_paths) + "\n" if archive_paths else "",
    )

    return [
        DefaultInfo(files = depset([output, objects_output], transitive = [object_depset])),
        OutputGroupInfo(
            allowlist = depset([output]),
            objects = depset([objects_output]),
            object_files = object_depset,
        ),
    ]

coverage_scope = rule(
    implementation = _coverage_scope_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_coverage_scope_aspect],
            doc = "Production cc_library targets whose transitive source files form the coverage scope.",
        ),
    },
    doc = "Derives a source-file allowlist and baseline-objects manifest from cc_library dep graphs.",
)
