/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_UTILS_CTC_H
#define FLATCFG_UTILS_CTC_H

#ifdef __CTC__
    #define FLATCFG_SKIPCOV_BEGIN() \
        _Pragma("CTC SKIP")         \
        static_assert(true, "")
#else
    #define FLATCFG_SKIPCOV_BEGIN() \
        static_assert(true, "")
#endif

#ifdef __CTC__
    #define FLATCFG_SKIPCOV_END() \
        _Pragma("CTC ENDSKIP")    \
        static_assert(true, "")
#else
    #define FLATCFG_SKIPCOV_END() \
        static_assert(true, "")
#endif

#endif  // FLATCFG_UTILS_CTC_H
