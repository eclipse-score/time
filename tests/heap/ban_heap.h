/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/
//
/// @file ban_heap.h
/// @note Usage
///       
///       Ensure that LD_PRELOAD is set:
///             export LD_PRELOAD=/path/to/libban_heap.so   
///
///       The function ara::core::Initialize behaves normally, additionally 
///       calls of an allocation function lead to an abnormal program 
///       termination after the call.
///       The function ara::core::Deinitialize behaves normally, additionally 
///       the restrictions on allocations are lifted.
// ==================================================================================

#ifndef BAN_HEAP_H_
#define BAN_HEAP_H_

namespace score {
namespace time {
namespace testing {

/// @brief Causes the program to abort when a allocation function is executed.
/// @note Do not use. Use ara::core::Initialize instead. This function is  
///       present for tool qualification purposes only.
/// @param abort : abort the program or display a message instead. 
void BanHeap(bool abort = true);

/// @brief Removes the restriction imposed by BanHeap.
/// @note Do not use. Use ara::core::Deinitialize instead.
///       This function is present for tool
///       qualification purposes only.
void PermitHeap();

}  // namespace testing
}  // namespace time
}  // namespace score

#endif /* BAN_HEAP_H_ */
