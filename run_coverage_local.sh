#!/bin/bash
# Script to run code coverage locally

set -e  # Exit on error

echo "Running code coverage analysis..."
bazel coverage //src/...

echo ""
echo "Generating HTML coverage report..."
OUTPUT_PATH=$(bazel info output_path)

if [ -f "${OUTPUT_PATH}/_coverage/_coverage_report.dat" ]; then

  genhtml "${OUTPUT_PATH}/_coverage/_coverage_report.dat" \
    -o=cpp_coverage \
    --show-details \
    --legend \
    --function-coverage \
    --branch-coverage

  echo ""
  echo "  Coverage report generated successfully!"
  echo "  Location: cpp_coverage/"

  if command -v xdg-open &> /dev/null; then
    xdg-open cpp_coverage/index.html
  fi
else
  echo "Error: Coverage data file not found at ${OUTPUT_PATH}/_coverage/_coverage_report.dat"
  exit 1
fi
