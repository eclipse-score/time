# Documentation Generation for SCORE Time Synchronization

This guide explains how to automatically generate documentation (.rst and .puml files) from C++ source code.

## Quick Start

### Generate All Documentation

```bash
# Using bazel (recommended)
bazel run //:doc-gen -- --all

# Or directly with Python
python3 tools/generate_docs.py --all
```

### Build HTML Documentation

```bash
bazel run //:docs
```

### View Documentation

Open `bazel-bin/docs/docs/html/index.html` in your browser.

## Available Options

The documentation generator supports several options:

```bash
# Generate only API documentation
bazel run //:doc-gen -- --api

# Generate only architecture diagrams
bazel run //:doc-gen -- --arch

# Generate only sequence diagrams
bazel run //:doc-gen -- --seq

# Generate all and update index.rst
bazel run //:doc-gen -- --all --update-index
```

## Generated Files

After running `--all`, you will have:

```
docs/
├── api/
│   ├── index.rst               # API overview
│   ├── core_types.rst          # SharedState, SyncLogEntry, enums
│   ├── ipc.rst                 # ShmRegion, seqlock functions
│   └── utilities.rst           # ClockNs, ParseInteger, PthreadLockGuard
├── diagrams/
│   ├── index.rst               # Diagrams overview
│   ├── architecture.puml       # High-level system architecture
│   ├── class_relationships.puml # Core class diagram
│   └── sequence_gptp_sync.puml # gPTP synchronization workflow
└── index.rst (updated)         # Main documentation index
```

## Using with Claude Code

You can use the `/doc-gen` skill with Claude Code:

```
User: Generate documentation for score_time
Assistant: [Automatically runs documentation generator]
```

The skill is defined in `.claude/skills/doc_generator_skill.md`.

## Customization

### Adding New Diagrams

To add a new PlantUML diagram:

1. Create the `.puml` file in `docs/diagrams/`
2. Reference it in `docs/diagrams/index.rst`:

```rst
.. uml:: diagrams/my_new_diagram.puml
   :caption: My New Diagram Description
```

### Extending API Documentation

The generator automatically extracts classes from:
- `src/common/include/score_time/` (public API)
- `src/tsyncd/engine/` (daemon implementation)

To improve extraction:
1. Add Doxygen-style comments to your C++ code:

```cpp
/**
 * @brief Brief description of the class
 *
 * Detailed description here.
 */
class MyClass {
    /**
     * @brief Brief description of method
     * @param arg1 Description of arg1
     * @return Description of return value
     */
    int MyMethod(int arg1);
};
```

2. Re-run the generator:

```bash
bazel run //:doc-gen -- --api
```

### Manual Tweaks

After auto-generation, you can manually edit:
- `docs/api/*.rst` files to add examples
- `docs/diagrams/*.puml` files to refine visual layout
- `docs/index.rst` to reorganize structure

**Important:** Remember that re-running the generator will overwrite your changes. Consider:
1. Committing generated files to git
2. Using separate `*_custom.rst` files for manual content
3. Editing `tools/generate_docs.py` to customize templates

## Integration with CI/CD

To ensure documentation stays up-to-date:

### GitHub Actions Example

```yaml
name: Documentation

on:
  push:
    branches: [main]
  pull_request:

jobs:
  build-docs:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Generate Documentation
        run: bazel run //:doc-gen -- --all

      - name: Build HTML Documentation
        run: bazel run //:docs

      - name: Upload Documentation
        uses: actions/upload-artifact@v3
        with:
          name: documentation
          path: bazel-bin/docs/docs/html/
```

### Pre-commit Hook

Add to `.git/hooks/pre-commit`:

```bash
#!/bin/bash
# Regenerate docs before commit
bazel run //:doc-gen -- --all
git add docs/
```

## Troubleshooting

### "Module 'sphinx_needs' not found"

Ensure you have the required Sphinx extensions:

```bash
pip install sphinx sphinx-design sphinx-needs sphinxcontrib-plantuml
```

### "PlantUML diagrams not rendering"

Install PlantUML:

```bash
# Ubuntu/Debian
sudo apt-get install plantuml

# macOS
brew install plantuml
```

Configure path in `docs/conf.py`:

```python
plantuml = 'java -jar /path/to/plantuml.jar'
```

### "Cannot find header file"

Check that the paths in `tools/generate_docs.py` match your project structure:

```python
include_dirs = [
    project_root / "src" / "common" / "include" / "score_time",
    project_root / "src" / "tsyncd" / "engine",
]
```

## Contributing

When adding new C++ components:

1. Write Doxygen comments for public APIs
2. Run `/doc-gen --all` to update documentation
3. Review generated `.rst` and `.puml` files
4. Add custom examples if needed
5. Commit both code and documentation

## References

- [Sphinx Documentation](https://www.sphinx-doc.org/)
- [PlantUML Guide](https://plantuml.com/)
- [ReStructuredText Primer](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html)
- [Sphinx C++ Domain](https://www.sphinx-doc.org/en/master/usage/restructuredtext/domains.html#the-c-domain)
