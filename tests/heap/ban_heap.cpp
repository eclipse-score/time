/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "ban_heap.h"

#include <dlfcn.h>
#include <sys/mman.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <new>

#include "score/time/common/Abort.h"

//!!#include "ara/core/initialization.h"
#include "score/result/result.h"

using score::time::common::logFatalAndAbort;

#ifdef __QNX__
#define __EXT_QNX
#endif

// "mmap" needs "NOFD" for QNX. If "NOFD" is not defined, set it to "0". This is the value Linux expects.
#ifndef NOFD
#define NOFD 0
#endif

namespace score {
namespace time {

namespace testing {

std::atomic_bool is_heap_banned{false};
std::atomic_bool abort_flag{true};

void BanHeap(bool abort) {
    is_heap_banned.store(true);
    abort_flag.store(abort);
}

void PermitHeap() {
    is_heap_banned.store(false);
}

/// @brief Checks the is_heap_banned flag
void CheckHeapBanFlag(const char* msg) {
    if (is_heap_banned.load()) {
        if (abort_flag.load()) {
            logFatalAndAbort(msg);
        } else {
            // For tool qualification purposes
            std::cout << msg << std::endl;
        }
    }
}

}  // namespace testing

/// @brief pointers of the original function of the core-types lib
static score::ResultBlank (*_ld_preload_original_ara_core_initialize)(void) = nullptr;
static score::ResultBlank (*_ld_preload_original_ara_core_deinitialize)(void) = nullptr;

/// @brief load all pointer from the core-types lib to the function pointers
static void CoreInit() {
    // _ld_preload_original_ara_core_initialize =
    //     (score::ResultBlank(*)())dlsym(RTLD_NEXT, "_ZN3ara4core10InitializeEv");
    // _ld_preload_original_ara_core_deinitialize =
    //     (score::ResultBlank(*)())dlsym(RTLD_NEXT, "_ZN3ara4core12DeinitializeEv");
    // if (!_ld_preload_original_ara_core_initialize || !_ld_preload_original_ara_core_deinitialize) {
    //     std::fputs("libheap_ban.so: Unable to hook initialization and shutdown functions!\n", stderr);
    //     std::fputs(dlerror(), stderr);
    //     std::exit(-1);
    // }
}

// ResultBlank Initialize() noexcept {
//     if (!_ld_preload_original_ara_core_initialize) { /* check if CoreInit was called earlier */
//         CoreInit();
//     }
//     auto ret{_ld_preload_original_ara_core_initialize()};
//     heap::BanHeap();
//     return ret;
// }

// ResultBlank Deinitialize() noexcept {
//     if (!_ld_preload_original_ara_core_deinitialize) { /* check if CoreInit was called earlier */
//         CoreInit();
//     }
//     heap::PermitHeap();
//     auto ret{_ld_preload_original_ara_core_deinitialize()};
//     return ret;
// }

}  // namespace time
}  // namespace score

using score::time::testing::CheckHeapBanFlag;

#ifdef __cplusplus
extern "C" {
#endif

static int alloc_init_pending = 0; /**< != 0 for alloc init is pending */

/**
 * @brief pointers of the original functions of the clib
 *
 */
static void* (*_ld_preload_original_malloc)(size_t) = NULL;
static void* (*_ld_preload_original_realloc)(void*, size_t) = NULL;
static void* (*_ld_preload_original_calloc)(size_t, size_t) = NULL;
static void* (*_ld_preload_original_aligned_alloc)(size_t, size_t) = NULL;

/**
 * @brief load all pointer from the Clib to the function pointers
 */
static void AllocInit(void) {
    alloc_init_pending = 1;
    _ld_preload_original_malloc = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    _ld_preload_original_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
    _ld_preload_original_calloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    _ld_preload_original_aligned_alloc = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "aligned_alloc");
    if (!_ld_preload_original_malloc || !_ld_preload_original_realloc || !_ld_preload_original_calloc ||
        !_ld_preload_original_aligned_alloc) {
        std::fputs("libheap_ban.so: Unable to hook allocation!\n", stderr);
        std::fputs(dlerror(), stderr);
        std::exit(-1);
    }
    alloc_init_pending = 0;
}

/*
 * @brief InitialAllocInternal allocation function used by early calls
 * @param size : size of memory to allocate
 * @return pointer to allocated memory
 *
 */
static void* InitialAllocInternal(size_t size) {
    /* Anonymous mapping ensures that pages are zero'd */
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, NOFD, 0);
    if (MAP_FAILED == ptr) {
        perror("InitialAllocInternal mmap failed");
        return NULL;
    }
    return ptr;
}

/**
 * @brief "_dl_alloc" should use internal memory, so is overloaded here
 * @param size : size of memory to allocate
 */
void* _dl_alloc(size_t size) {
    return InitialAllocInternal(size);
}

/**
 * @brief overloaded realloc function
 * @param ptr  : old pointer to memory
 * @param size : size of memory to allocate
 * @return pointer to the memory
 */
void* realloc(void* ptr, size_t size) {
    CheckHeapBanFlag("[testing][ban_heap] void* realloc(void* ptr, size_t size) called.");
    if (!_ld_preload_original_realloc) { /* check if alloc_init was called earlier */
        AllocInit();
    }
    return _ld_preload_original_realloc(ptr, size);
}

/**
 * @brief overloaded malloc function
 * @param size : size of memory to allocate
 * @return pointer to the memory
 */
void* malloc(size_t size) {
    CheckHeapBanFlag("[testing][ban_heap] void* malloc(size_t size) called.");
    if (alloc_init_pending) {
        return InitialAllocInternal(size);
    }
    if (!_ld_preload_original_malloc) { /* check if AllocInit was called earlier */
        AllocInit();
    }

    return _ld_preload_original_malloc(size);
}

/**
 * @brief overloaded calloc function
 * @param nmemb : number of elements to allocate
 * @param size : size of one element to allocate
 * @return pointer to the memory
 */
void* calloc(size_t nmemb, size_t size) {
    CheckHeapBanFlag("[testing][ban_heap] void* calloc(size_t nmemb, size_t size) called.");
    /* check if alloc init is ongoing on another thread */
    if (alloc_init_pending) {
        void* result = InitialAllocInternal(nmemb * size);
        std::memset(result, 0, nmemb * size);
        return result;
    }

    if (!_ld_preload_original_calloc) { /* check if AllocInit was called earlier */
        AllocInit();
    }
    return _ld_preload_original_calloc(nmemb, size);
}

/**
 * @brief overloaded aligned_alloc function
 * @param alignment : specifies the alignment. Must be a valid alignment supported by the implementation.
 * @param size : number of bytes to allocate. An integral multiple of alignment.
 * @return pointer to the memory
 */
void* aligned_alloc(size_t alignment, size_t size) {
    CheckHeapBanFlag("[testing][ban_heap] void *aligned_alloc( size_t alignment, size_t size)");
    if (alloc_init_pending) {
        return InitialAllocInternal(size);
    }
    if (!_ld_preload_original_aligned_alloc) { /* check if AllocInit was called earlier */
        AllocInit();
    }

    return _ld_preload_original_aligned_alloc(alignment, size);
}

#ifdef __cplusplus
}
#endif

#if __cplusplus > 201402L
// replaceable allocation functions since C++17
void* operator new(std::size_t count, std::align_val_t al) {
    CheckHeapBanFlag("[testing][ban_heap] void* operator new(std::size_t count, std::align_val_t al) called.");
    return aligned_alloc(static_cast<std::size_t>(al), count);
}

void* operator new[](std::size_t count, std::align_val_t al) {
    CheckHeapBanFlag("[testing][ban_heap] void* operator new[](std::size_t count, std::align_val_t al) called.");
    return aligned_alloc(static_cast<std::size_t>(al), count);
}

void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t&) {
    CheckHeapBanFlag(
        "[testing][ban_heap] void* operator new(std::size_t count, std::align_val_t al, const std::nothrow_t&) called.");
    return aligned_alloc(static_cast<std::size_t>(al), count);
}

void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t&) {
    CheckHeapBanFlag(
        "[testing][ban_heap] void* operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t&) called.");
    return aligned_alloc(static_cast<std::size_t>(al), count);
}

// replaceable usual deallocation functions since C++17
void operator delete(void* ptr, std::align_val_t) noexcept {
    std::free(ptr);
}
void operator delete[](void* ptr, std::align_val_t) noexcept {
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    std::free(ptr);
}
#endif

// replaceable allocation functions
void* operator new(std::size_t count) {
    CheckHeapBanFlag("[testing][ban_heap] void* operator new (std::size_t count) called.");
    return malloc(count);
}
void* operator new[](std::size_t count) {
    CheckHeapBanFlag("[testing][ban_heap] void* operator new[](std::size_t count) called.");
    return malloc(count);
}

// replaceable non-throwing allocation functions
void* operator new(std::size_t count, const std::nothrow_t& tag) {
    static_cast<void>(tag);
    CheckHeapBanFlag("[testing][ban_heap] void* operator new(std::size_t count, const std::nothrow_t& tag) called.");
    return malloc(count);
}

void* operator new[](std::size_t count, const std::nothrow_t& tag) {
    static_cast<void>(tag);
    CheckHeapBanFlag("[testing][ban_heap] void* operator new[](std::size_t count, const std::nothrow_t& tag) called.");
    return malloc(count);
}

// replaceable usual deallocation functions
void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}
