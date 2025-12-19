/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_FLATCFG_EXPORT_H
#define SCORE_FLATCFG_EXPORT_H

#ifdef SCORE_FLATCFG_STATIC_DEFINE
#  define SCORE_FLATCFG_EXPORT
#  define SCORE_FLATCFG_NO_EXPORT
#else
#  ifndef SCORE_FLATCFG_EXPORT
#    ifdef SCORE_FLATcfg_lib_EXPORTS
        /* We are building this library */
#      define SCORE_FLATCFG_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define SCORE_FLATCFG_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef SCORE_FLATCFG_NO_EXPORT
#    define SCORE_FLATCFG_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef SCORE_FLATCFG_DEPRECATED
#  define SCORE_FLATCFG_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef SCORE_FLATCFG_DEPRECATED_EXPORT
#  define SCORE_FLATCFG_DEPRECATED_EXPORT SCORE_FLATCFG_EXPORT SCORE_FLATCFG_DEPRECATED
#endif

#ifndef SCORE_FLATCFG_DEPRECATED_NO_EXPORT
#  define SCORE_FLATCFG_DEPRECATED_NO_EXPORT SCORE_FLATCFG_NO_EXPORT SCORE_FLATCFG_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SCORE_FLATCFG_NO_DEPRECATED
#    define SCORE_FLATCFG_NO_DEPRECATED
#  endif
#endif

#endif /* SCORE_FLATCFG_EXPORT_H */
