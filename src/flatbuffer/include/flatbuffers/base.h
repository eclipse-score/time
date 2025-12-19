/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATBUFFERS_BASE_H_
#define FLATBUFFERS_BASE_H_

// clang-format off

// Add to the end of a macro definition to force using a macro as a statement
#define FLATBUFFERS_REQUIRE_SEMICOLON static_assert(true, "")

// Necessary because _Pragma("x" "y") won't work directly
#define FLATBUFFERS_DO_PRAGMA_(x) _Pragma(#x)
#define FLATBUFFERS_DO_PRAGMA(x) FLATBUFFERS_DO_PRAGMA_(x)

// #ifdef __CTC__
//   #define FLATBUFFERS_CTC_ANNOTATION(justification_literal) FLATBUFFERS_DO_PRAGMA(CTC ANNOTATION justification_literal) FLATBUFFERS_REQUIRE_SEMICOLON
// #else
  #define FLATBUFFERS_CTC_ANNOTATION(justification_literal) FLATBUFFERS_REQUIRE_SEMICOLON
// #endif

#ifdef __CTC__
  #define FLATBUFFERS_EXCLUDE_COVERAGE_START(justification_literal) \
    FLATBUFFERS_CTC_ANNOTATION(justification_literal); _Pragma("CTC SKIP") FLATBUFFERS_REQUIRE_SEMICOLON
#else
  #define FLATBUFFERS_EXCLUDE_COVERAGE_START(justification_literal) FLATBUFFERS_REQUIRE_SEMICOLON
#endif

#ifdef __CTC__
  #define FLATBUFFERS_EXCLUDE_COVERAGE_END() _Pragma("CTC ENDSKIP") FLATBUFFERS_REQUIRE_SEMICOLON
#else
  #define FLATBUFFERS_EXCLUDE_COVERAGE_END() FLATBUFFERS_REQUIRE_SEMICOLON
#endif

// If activate should be declared and included first.
#if defined(FLATBUFFERS_MEMORY_LEAK_TRACKING) && \
    defined(_MSC_VER) && defined(_DEBUG)
  // The _CRTDBG_MAP_ALLOC inside <crtdbg.h> will replace
  // calloc/free (etc) to its debug version using #define directives.
  #define _CRTDBG_MAP_ALLOC
  #include <stdlib.h>
  #include <crtdbg.h>
  // Replace operator new by trace-enabled version.
  #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
  #define new DEBUG_NEW
#endif

#if !defined(FLATBUFFERS_ASSERT)
#define FLATBUFFERS_ASSERT static_cast<void>
#elif defined(FLATBUFFERS_ASSERT_INCLUDE)
// Include file with forward declaration
#include FLATBUFFERS_ASSERT_INCLUDE
#endif

#ifndef ARDUINO
#include <cstdint>
#endif

#include <cstddef>
#include <cstdlib>
#include <cstring>

#if defined(ARDUINO) && !defined(ARDUINOSTL_M_H)
  #include <utility.h>
#else
  #include <utility>
#endif

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <flatbuffers/stl_emulation.h>

#if defined(__ICCARM__)
#include <intrinsics.h>
#endif

// Note the __clang__ check is needed, because clang presents itself
// as an older GNUC compiler (4.2).
// Clang 3.3 and later implement all of the ISO C++ 2011 standard.
// Clang 3.4 and later implement all of the ISO C++ 2014 standard.
// http://clang.llvm.org/cxx_status.html

// Note the MSVC value '__cplusplus' may be incorrect:
// The '__cplusplus' predefined macro in the MSVC stuck at the value 199711L,
// indicating (erroneously!) that the compiler conformed to the C++98 Standard.
// This value should be correct starting from MSVC2017-15.7-Preview-3.
// The '__cplusplus' will be valid only if MSVC2017-15.7-P3 and the `/Zc:__cplusplus` switch is set.
// Workaround (for details see MSDN):
// Use the _MSC_VER and _MSVC_LANG definition instead of the __cplusplus  for compatibility.
// The _MSVC_LANG macro reports the Standard version regardless of the '/Zc:__cplusplus' switch.

#if defined(__GNUC__) && !defined(__clang__)
  #define FLATBUFFERS_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
  #define FLATBUFFERS_GCC 0
#endif

#if defined(__clang__)
  #define FLATBUFFERS_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#else
  #define FLATBUFFERS_CLANG 0
#endif

// The wire format uses a little endian encoding (since that's efficient for
// the common platforms).
#if defined(__s390x__)
  #define FLATBUFFERS_LITTLEENDIAN 0
#endif // __s390x__
#if !defined(FLATBUFFERS_LITTLEENDIAN)
  #if defined(__GNUC__) || defined(__clang__) || defined(__ICCARM__)
    #if (defined(__BIG_ENDIAN__) || \
         (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
      #define FLATBUFFERS_LITTLEENDIAN 0
    #else
      #define FLATBUFFERS_LITTLEENDIAN 1
    #endif // __BIG_ENDIAN__
  #elif defined(_MSC_VER)
    #if defined(_M_PPC)
      #define FLATBUFFERS_LITTLEENDIAN 0
    #else
      #define FLATBUFFERS_LITTLEENDIAN 1
    #endif
  #else
    #error Unable to determine endianness, define FLATBUFFERS_LITTLEENDIAN.
  #endif
#endif // !defined(FLATBUFFERS_LITTLEENDIAN)

#define FLATBUFFERS_VERSION_MAJOR 25
#define FLATBUFFERS_VERSION_MINOR 2
#define FLATBUFFERS_VERSION_REVISION 10
//SCORE Extension start
#define FLATBUFFERS_VERSION_SCORE_MAJOR 0
#define FLATBUFFERS_VERSION_SCORE_MINOR 1
//SCORE Extension end
#define FLATBUFFERS_STRING_EXPAND(X) #X
#define FLATBUFFERS_STRING(X) FLATBUFFERS_STRING_EXPAND(X)
namespace flatbuffers {
  // Returns version as string  "MAJOR.MINOR.REVISION".
  const char* FLATBUFFERS_VERSION();
}

// Available since C++14 at the latest.
#define FLATBUFFERS_FINAL_CLASS final
#define FLATBUFFERS_OVERRIDE override
#define FLATBUFFERS_VTABLE_UNDERLYING_TYPE : flatbuffers::voffset_t
#define FLATBUFFERS_CONSTEXPR constexpr
#define FLATBUFFERS_CONSTEXPR_CPP14 FLATBUFFERS_CONSTEXPR
#define FLATBUFFERS_NOEXCEPT noexcept
#define FLATBUFFERS_DELETE_FUNC(func) func = delete;

#ifndef FLATBUFFERS_HAS_STRING_VIEW
  // Only provide flatbuffers::string_view if __has_include can be used
  // to detect a header that provides an implementation
  #if defined(__has_include)
    // Check for std::string_view (in c++17)
    #if __has_include(<string_view>) && (__cplusplus >= 201606 || (defined(_HAS_CXX17) && _HAS_CXX17))
      #include <string_view>
      namespace flatbuffers {
        using string_view = std::string_view;
      }
      #define FLATBUFFERS_HAS_STRING_VIEW 1
    // Check for std::experimental::string_view (in c++14, compiler-dependent)
    #elif __has_include(<experimental/string_view>) && (__cplusplus >= 201411)
      #include <experimental/string_view>
      namespace flatbuffers {
        using string_view = std::experimental::string_view;
      }
      #define FLATBUFFERS_HAS_STRING_VIEW 1
    // Check for absl::string_view
    #elif __has_include("absl/strings/string_view.h")
      #include "absl/strings/string_view.h"
      namespace flatbuffers {
        using string_view = absl::string_view;
      }
      #define FLATBUFFERS_HAS_STRING_VIEW 1
    #endif
  #endif // __has_include
#endif // !FLATBUFFERS_HAS_STRING_VIEW

// Available since C++11.
#define FLATBUFFERS_HAS_NEW_STRTOD 1

#ifndef FLATBUFFERS_LOCALE_INDEPENDENT
  // Enable locale independent functions {strtof_l, strtod_l,strtoll_l, strtoull_l}.
  // They are part of the POSIX-2008 but not part of the C/C++ standard.
  // GCC/Clang have definition (_XOPEN_SOURCE>=700) if POSIX-2008.
  #if ((defined(_MSC_VER) && _MSC_VER >= 1800)            || \
       (defined(_XOPEN_SOURCE) && (_XOPEN_SOURCE>=700)))
    #define FLATBUFFERS_LOCALE_INDEPENDENT 1
  #else
    #define FLATBUFFERS_LOCALE_INDEPENDENT 0
  #endif
#endif  // !FLATBUFFERS_LOCALE_INDEPENDENT

// Suppress Undefined Behavior Sanitizer (recoverable only). Usage:
// - __supress_ubsan__("undefined")
// - __supress_ubsan__("signed-integer-overflow")
#if defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >=7))
  #define __supress_ubsan__(type) __attribute__((no_sanitize(type)))
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 409)
  #define __supress_ubsan__(type) __attribute__((no_sanitize_undefined))
#else
  #define __supress_ubsan__(type)
#endif

// This is constexpr function used for checking compile-time constants.
// Avoid `#pragma warning(disable: 4127) // C4127: expression is constant`.
template<typename T> FLATBUFFERS_CONSTEXPR inline bool IsConstTrue(T t) {
  return !!t;
}

// Enable C++ attribute [[]] if std:c++17 or higher.
#if ((__cplusplus >= 201703L) \
    || (defined(_MSVC_LANG) &&  (_MSVC_LANG >= 201703L)))
  // All attributes unknown to an implementation are ignored without causing an error.
  #define FLATBUFFERS_ATTRIBUTE(attr) [[attr]]

  #define FLATBUFFERS_FALLTHROUGH() [[fallthrough]]
#else
  #define FLATBUFFERS_ATTRIBUTE(attr)

  #if FLATBUFFERS_CLANG >= 30800
    #define FLATBUFFERS_FALLTHROUGH() [[clang::fallthrough]]
  #elif FLATBUFFERS_GCC >= 70300
    #define FLATBUFFERS_FALLTHROUGH() [[gnu::fallthrough]]
  #else
    #define FLATBUFFERS_FALLTHROUGH()
  #endif
#endif

/// @endcond

/// @file
namespace flatbuffers {

/// @cond FLATBUFFERS_INTERNAL
// Our default offset / size type, 32bit on purpose on 64bit systems.
// Also, using a consistent offset type maintains compatibility of serialized
// offset values between 32bit and 64bit systems.
using uoffset_t = uint32_t;

// Signed offsets for references that can go in both directions.
using soffset_t = int32_t;

// Offset/index used in v-tables, can be changed to uint8_t in
// format forks to save a bit of space if desired.
using voffset_t = uint16_t;

using largest_scalar_t = uintmax_t;

// In 32bits, this evaluates to 2GB - 1
#define FLATBUFFERS_MAX_BUFFER_SIZE ((1ULL << (sizeof(::flatbuffers::soffset_t) * 8U - 1U)) - 1U)

// We support aligning the contents of buffers up to this size.
#define FLATBUFFERS_MAX_ALIGNMENT 16U

#if defined(_MSC_VER)
  #pragma warning(push)
  #pragma warning(disable: 4127) // C4127: conditional expression is constant
#endif

template<typename T> T EndianSwap(T t) {
  #if defined(_MSC_VER)
    #define FLATBUFFERS_BYTESWAP16 _byteswap_ushort
    #define FLATBUFFERS_BYTESWAP32 _byteswap_ulong
    #define FLATBUFFERS_BYTESWAP64 _byteswap_uint64
  #elif defined(__ICCARM__)
    #define FLATBUFFERS_BYTESWAP16 __REV16
    #define FLATBUFFERS_BYTESWAP32 __REV
    #define FLATBUFFERS_BYTESWAP64(x) \
       ((__REV(static_cast<uint32_t>(x >> 32U))) | (static_cast<uint64_t>(__REV(static_cast<uint32_t>(x)))) << 32U)
  #else
    #if defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ < 408 && !defined(__clang__)
      // __builtin_bswap16 was missing prior to GCC 4.8.
      #define FLATBUFFERS_BYTESWAP16(x) \
        static_cast<uint16_t>(__builtin_bswap32(static_cast<uint32_t>(x) << 16))
    #else
      #define FLATBUFFERS_BYTESWAP16 __builtin_bswap16
    #endif
    #define FLATBUFFERS_BYTESWAP32 __builtin_bswap32
    #define FLATBUFFERS_BYTESWAP64 __builtin_bswap64
  #endif
  if (sizeof(T) == 1U) {   // Compile-time if-then's.
    return t;
  } else if (sizeof(T) == 2U) {
    uint16_t i;
    std::memcpy(&i, &t, sizeof(T));
    i = FLATBUFFERS_BYTESWAP16(i);
    std::memcpy(&t, &i, sizeof(T));
    return t;
  } else if (sizeof(T) == 4U) {
    uint32_t i;
    std::memcpy(&i, &t, sizeof(T));
    i = FLATBUFFERS_BYTESWAP32(i);
    std::memcpy(&t, &i, sizeof(T));
    return t;
  } else if (sizeof(T) == 8U) {
    uint64_t i;
    std::memcpy(&i, &t, sizeof(T));
    i = FLATBUFFERS_BYTESWAP64(i);
    std::memcpy(&t, &i, sizeof(T));
    return t;
  } else {
    FLATBUFFERS_ASSERT(0);
    return t;
  }
}

#if defined(_MSC_VER)
  #pragma warning(pop)
#endif


template<typename T> T EndianScalar(T t) {
  #if FLATBUFFERS_LITTLEENDIAN
    return t;
  #else
    return EndianSwap(t);
  #endif
}

template<typename T>
// UBSAN: C++ aliasing type rules, see std::bit_cast<> for details.
__supress_ubsan__("alignment")
T ReadScalar(const void *p) {
  // RULECHECKER_comment(1, 2, check_reinterpret_cast, "Implementations that don't use cast affect memory usage and performance degradation.", true);
  return EndianScalar(*reinterpret_cast<const T *>(p));
}

template<typename T>
// UBSAN: C++ aliasing type rules, see std::bit_cast<> for details.
__supress_ubsan__("alignment")
void WriteScalar(void *p, T t) {
  // RULECHECKER_comment(1, 2, check_reinterpret_cast, "Implementations that don't use cast affect memory usage and performance degradation.", true);
  *reinterpret_cast<T *>(p) = EndianScalar(t);
}

template<typename T> struct Offset;
template<typename T> __supress_ubsan__("alignment") void WriteScalar(void *p, Offset<T> t) {
  // RULECHECKER_comment(1, 2, check_reinterpret_cast, "Implementations that don't use cast affect memory usage and performance degradation.", true);
  *reinterpret_cast<uoffset_t *>(p) = EndianScalar(t.o);
}

// Computes how many bytes you'd have to pad to be able to write an
// "scalar_size" scalar if the buffer had grown to "buf_size" (downwards in
// memory).
__supress_ubsan__("unsigned-integer-overflow")
inline size_t PaddingBytes(size_t buf_size, size_t scalar_size) {
  return ((~buf_size) + 1U) & (scalar_size - 1U);
}

}  // namespace flatbuffers
#endif  // FLATBUFFERS_BASE_H_
