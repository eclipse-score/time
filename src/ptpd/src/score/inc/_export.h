/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_CRC_EXPORT_H
#define SCORE_CRC_EXPORT_H

#ifdef SCORE_CRC_STATIC_DEFINE
#  define SCORE_CRC_EXPORT
#  define SCORE_CRC_NO_EXPORT
#else
#  ifndef SCORE_CRC_EXPORT
#    ifdef SCORE_CRC_lib_EXPORTS
        /* We are building this library */
#      define SCORE_CRC_EXPORT __attribute__((visibility("default")))
#    else
        /* We are using this library */
#      define SCORE_CRC_EXPORT __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef SCORE_CRC_NO_EXPORT
#    define SCORE_CRC_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef SCORE_CRC_DEPRECATED
#  define SCORE_CRC_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef SCORE_CRC_DEPRECATED_EXPORT
#  define SCORE_CRC_DEPRECATED_EXPORT SCORE_CRC_EXPORT SCORE_CRC_DEPRECATED
#endif

#ifndef SCORE_CRC_DEPRECATED_NO_EXPORT
#  define SCORE_CRC_DEPRECATED_NO_EXPORT SCORE_CRC_NO_EXPORT SCORE_CRC_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SCORE_CRC_NO_DEPRECATED
#    define SCORE_CRC_NO_DEPRECATED
#  endif
#endif

#endif /* SCORE_CRC_EXPORT_H */
