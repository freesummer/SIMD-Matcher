#ifndef __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_NODE_BASE__
#define __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_NODE_BASE__

#include <iostream>
#include <map>
#include <string>
#include <bitset>

#include <hot/commons/include/hot/commons/NodeAllocationInformation.hpp>

#include "hot/single-threaded/include/hot/singlethreaded/MemoryPool.hpp"
#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedNodeBaseInterface.hpp"
#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedChildPointerInterface.hpp"

constexpr uint32_t ALL_ENTRIES_USED = UINT32_MAX; //32 1 bits

extern uint32_t wildcard_affected_entry_mask;

namespace hot { namespace singlethreaded {

inline MemoryPool<uint64_t, MAXIMUM_NODE_SIZE_IN_LONGS>* HOTSingleThreadedNodeBase::getMemoryPool() {
	static MemoryPool<uint64_t, MAXIMUM_NODE_SIZE_IN_LONGS> memoryPool {};
	return &memoryPool;
}

//HOTSingleThreadedNodeBase::HOTSingleThreadedNodeBase(uint16_t const level, hot::commons::NodeAllocationInformation const & nodeAllocationInformation)
//	: mFirstChildPointer(reinterpret_cast<HOTSingleThreadedChildPointer*>(reinterpret_cast<char*>(this) + nodeAllocationInformation.mPointerOffset)), mUsedEntriesMask(nodeAllocationInformation.mEntriesMask), mHeight(level) {
//}

HOTSingleThreadedNodeBase::HOTSingleThreadedNodeBase(uint16_t const level, hot::commons::NodeAllocationInformation const & nodeAllocationInformation)
  : mFirstChildPointer(reinterpret_cast<HOTSingleThreadedChildPointer*>(reinterpret_cast<char*>(this) + nodeAllocationInformation.mPointerOffset)), mUsedEntriesMask(nodeAllocationInformation.mEntriesMask), mHeight(level) {
}

inline __attribute__((always_inline)) size_t HOTSingleThreadedNodeBase::getNumberEntries() const {
	return __builtin_popcount(mUsedEntriesMask);
}

inline __attribute__((always_inline)) bool HOTSingleThreadedNodeBase::isFull() const {
	return mUsedEntriesMask == ALL_ENTRIES_USED;
}

inline __attribute__((always_inline)) HOTSingleThreadedChildPointer const * HOTSingleThreadedNodeBase::toResult( uint32_t const resultMask) const {
	return getPointers() + toResultIndex(resultMask);
}

inline __attribute__((always_inline)) HOTSingleThreadedChildPointer* HOTSingleThreadedNodeBase::toResult( uint32_t const resultMask) {
	size_t const resultIndex = toResultIndex(resultMask);
	return getPointers() + resultIndex;
}

inline __attribute__((always_inline)) unsigned int HOTSingleThreadedNodeBase::toResultIndex( uint32_t resultMask ) const {
  #ifdef PING_DEBUG3
  std::cout << "toResultIndex function" << std::endl;
#endif
	assert(resultMask != 0);
   #ifdef PING_DEBUG3
  std::cout << "mUsedEntriesMask = " << std::bitset<32>(mUsedEntriesMask) << std::endl;
  #endif
  uint32_t const resultMaskForChildsOnly = mUsedEntriesMask & resultMask;
  wildcard_affected_entry_mask = wildcard_affected_entry_mask & mUsedEntriesMask;
  #ifdef PING_DEBUG3
  std::cout << "wildcard_affected_entry_mask = " << std::bitset<32>(wildcard_affected_entry_mask) << std::endl;
  printf("toResultIndex = %d\n", hot::commons::getMostSignificantBitIndex(resultMaskForChildsOnly));
  #endif

  return hot::commons::getMostSignificantBitIndex(resultMaskForChildsOnly);
}

inline size_t HOTSingleThreadedNodeBase::getNumberAllocations() {
	return getMemoryPool()->getNumberAllocations();
}


inline HOTSingleThreadedChildPointer * HOTSingleThreadedNodeBase::getPointers()  {
	return mFirstChildPointer;
}

inline HOTSingleThreadedChildPointer const * HOTSingleThreadedNodeBase::getPointers() const {
	return mFirstChildPointer;
}

inline typename  HOTSingleThreadedNodeBase::iterator HOTSingleThreadedNodeBase::begin()
{
	return getPointers();
}

inline typename  HOTSingleThreadedNodeBase::iterator HOTSingleThreadedNodeBase::end()
{
	return getPointers() + getNumberEntries();
}

inline typename  HOTSingleThreadedNodeBase::const_iterator HOTSingleThreadedNodeBase::begin() const
{
	return getPointers();
}

inline typename  HOTSingleThreadedNodeBase::const_iterator HOTSingleThreadedNodeBase::end() const
{
	return getPointers() + getNumberEntries();
}

} }

#endif
