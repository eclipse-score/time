/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/
// ======================================================================================
/// @file pre_allocate.h
/// @note Usage
///
///       Ensure that LD_PRELOAD is set:
///             export LD_PRELOAD="/path/to/libban_heap.so /path/to/libpre_allocate.so"
// ======================================================================================

#ifndef PRE_ALLOCATE_H_
#define PRE_ALLOCATE_H_

//!! #include "ara/core/pmr/safe_memory_resource.h"

namespace score {
namespace core {
namespace heap {

/// @brief Memory pool
/// @tparam T type
/// @tparam Tag Tag of memory pool
/// @note see C++ named requirements: Allocator
template <class T, class Tag>
class PoolAllocator {
public:
    typedef T value_type;
    //!! typedef typename ara::core::pmr::GlobalSafeMemoryPool<Tag>
    //!!     TaggedMemoryPool;  // <--- this is the memory pool going to be used by PoolAllocator
    PoolAllocator() = default;
    template <class U>
    constexpr PoolAllocator(const PoolAllocator<U, Tag>&) noexcept {
    }
    T* allocate(std::size_t n) {
        //!! auto p = TaggedMemoryPool::get().allocate(n * sizeof(T));
        //!! return static_cast<T*>(p);
        return nullptr;
    }

    void deallocate(T* p, std::size_t) noexcept {
        //!! TaggedMemoryPool::get().deallocate(p);
    }
};

template <class T, class U, class Tag>
bool operator==(const PoolAllocator<T, Tag>&, const PoolAllocator<U, Tag>&) {
    return true;
}

template <class T, class U, class Tag>
bool operator!=(const PoolAllocator<T, Tag>&, const PoolAllocator<U, Tag>&) {
    return false;
}

class PoolTag {};

}  // namespace heap
}  // namespace core
}  // namespace score

#endif  // PRE_ALLOCATE_H_
