/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_COMMON_EXCLUDECOVERAGEADAPTER_H_
#define SCORE_TIME_COMMON_EXCLUDECOVERAGEADAPTER_H_

#ifdef __CTC__
#define EXCLUDE_COVERAGE_START(justification) _Pragma("CTC ANNOTATION justification") _Pragma("CTC SKIP")
#else
#define EXCLUDE_COVERAGE_START(justification)
#endif

#ifdef __CTC__
#define EXCLUDE_COVERAGE_END _Pragma("CTC ENDSKIP")
#else
#define EXCLUDE_COVERAGE_END
#endif

#endif  // SCORE_TIME_COMMON_EXCLUDECOVERAGEADAPTER_H_
