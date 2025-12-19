/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef STD_TYPES_H_
#define STD_TYPES_H_

#ifdef __cplusplus

#include <cstdint>

// Type aliases so that the defined APIs are not altered from the specification
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using boolean = bool;

#define NULL_PTR nullptr

#else // __cplusplus

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef bool boolean;

#define NULL_PTR ((void*)0)

#endif // __cplusplus

#define FALSE false
#define TRUE true

#endif /* STD_TYPES_H_ */
