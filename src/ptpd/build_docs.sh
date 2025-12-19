#!/bin/sh
#*******************************************************************************
# (c) 2025 ETAS GmbH. All rights reserved.
#********************************************************************************

set -eu

# Clean up old stuff
rm -rf _doc
mkdir -p  _doc

# Execute Doxygen
doxygen Doxyfile

# Build sphinx needs
sphinx-build -j auto -b needs . _doc/needs -d _doc_cache

# Build sphinx html
sphinx-build -j auto -b html . _doc/html -w _doc/sphinx_warnings.log -d _doc_cache
