/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/common/ExcludeCoverageAdapter.h"

EXCLUDE_COVERAGE_START(
    "Main function wrapper used for testing tsyncd_main with Unit Tests, it does not make sense to write unit tests for it.")

extern int tsyncd_main(int argc, char* argv[]);

// coverity[autosar_cpp14_a15_3_3_violation] No exceptions will occur.
int main(int argc, char* argv[]) {
    tsyncd_main(argc, argv);
}

EXCLUDE_COVERAGE_END