#ifndef __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_CHILD_POINTER__
#define __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_CHILD_POINTER__

#include <hot/commons/include/hot/commons/NodeParametersMapping.hpp>
#include <hot/commons/include/hot/commons/NodeType.hpp>

#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedChildPointerInterface.hpp"
#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedNode.hpp"

#include <algorithm>    // std::copy
#include<vector>
#include<iterator> // for back_inserter

namespace hot { namespace singlethreaded {

constexpr intptr_t NODE_ALGORITHM_TYPE_EXTRACTION_MASK = 0x7u;
constexpr intptr_t POINTER_AND_IS_LEAF_VALUE_MASK = 15u;
constexpr intptr_t POINTER_EXTRACTION_MASK = ~(POINTER_AND_IS_LEAF_VALUE_MASK);

template<hot::commons::NodeType  nodeAlgorithmType> inline auto HOTSingleThreadedChildPointer::castToNode(HOTSingleThreadedNodeBase const * node) {
  using DiscriminativeBitsRepresentationType = typename hot::commons::NodeTypeToNodeParameters<nodeAlgorithmType>::PartialKeyMappingType;
  using PartialKeyType = typename hot::commons::NodeTypeToNodeParameters<nodeAlgorithmType>::PartialKeyType;
  return reinterpret_cast<HOTSingleThreadedNode<DiscriminativeBitsRepresentationType, PartialKeyType> const *>(node);
}

template<hot::commons::NodeType  nodeAlgorithmType> inline auto HOTSingleThreadedChildPointer::castToNode(HOTSingleThreadedNodeBase * node) {
  using DiscriminativeBitsRepresentationType = typename hot::commons::NodeTypeToNodeParameters<nodeAlgorithmType>::PartialKeyMappingType;
  using PartialKeyType = typename hot::commons::NodeTypeToNodeParameters<nodeAlgorithmType>::PartialKeyType;
  return reinterpret_cast<HOTSingleThreadedNode<DiscriminativeBitsRepresentationType, PartialKeyType> *>(node);
}

template<typename Operation> inline __attribute__((always_inline)) auto HOTSingleThreadedChildPointer::executeForSpecificNodeType(bool const withPrefetch, Operation const & operation) const {
  HOTSingleThreadedNodeBase const * node = getNode();

  if(withPrefetch) {
    __builtin_prefetch(node);
    __builtin_prefetch(reinterpret_cast<char const*>(node) + 64);
    __builtin_prefetch(reinterpret_cast<char const*>(node) + 128);
    __builtin_prefetch(reinterpret_cast<char const*>(node) + 192);
  }

  switch(getNodeType()) {
  case hot::commons::NodeType ::SINGLE_MASK_8_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_8_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::SINGLE_MASK_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_16_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::SINGLE_MASK_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_32_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_8_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_8_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_32_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_16_BYTES_AND_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_16_BYTES_AND_16_BIT_PARTIAL_KEYS>(node));
  default: //hot::commons::NodeType ::MULTI_MASK_32_BYTES_AND_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_32_BYTES_AND_32_BIT_PARTIAL_KEYS>(node));
  }
}

template<typename Operation> inline __attribute__((always_inline)) auto HOTSingleThreadedChildPointer::executeForSpecificNodeType(bool const withPrefetch, Operation const & operation) {
#ifdef PING_DEBUG3
  std::cout << "-----executeForSpecificNodeType function:-----not const one--- " << std::endl;
#endif
  HOTSingleThreadedNodeBase * node = getNode();

  if(withPrefetch) {
    __builtin_prefetch(node);
    __builtin_prefetch(reinterpret_cast<char*>(node) + 64);
    __builtin_prefetch(reinterpret_cast<char*>(node) + 128);
    __builtin_prefetch(reinterpret_cast<char*>(node) + 192);
  }

  switch(getNodeType()) {
  case hot::commons::NodeType ::SINGLE_MASK_8_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_8_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::SINGLE_MASK_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_16_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::SINGLE_MASK_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::SINGLE_MASK_32_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_8_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_8_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_16_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_8_BYTES_AND_32_BIT_PARTIAL_KEYS>(node));
  case hot::commons::NodeType ::MULTI_MASK_16_BYTES_AND_16_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType ::MULTI_MASK_16_BYTES_AND_16_BIT_PARTIAL_KEYS>(node));  // type force redefine
  default: //hot::commons::NodeType ::MULTI_MASK_32_BYTES_AND_32_BIT_PARTIAL_KEYS:
    return operation(*castToNode<hot::commons::NodeType::MULTI_MASK_32_BYTES_AND_32_BIT_PARTIAL_KEYS>(node));
  }
}


inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer() : mPointer(reinterpret_cast<intptr_t>(nullptr)), mMask(uint64_t(0)), priority(uint64_t(0)), listSize(uint8_t(0)) {
#ifdef PING_DEBUG9
  printf("----------------empty initialize 0------------------\n");
#endif
}

inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(HOTSingleThreadedChildPointer const & other)
//  : mPointer(other.mPointer), mMask(other.mMask), priority(other.priority), entryList(other.entryList), listSize(other.listSize)
{
#ifdef PING_DEBUG7
  printf("-----------------other initialize------------------\n");
#endif
  mPointer = other.mPointer;
  mMask = other.mMask;
  priority = other.priority;
  entryList = other.entryList;
  listSize = other.listSize;
}




inline HOTSingleThreadedChildPointer & HOTSingleThreadedChildPointer::operator=(const HOTSingleThreadedChildPointer &other) {
#ifdef PING_DEBUG7
  printf("-----------------operator = initialize------------------\n");
#endif
  mPointer = other.mPointer;
  mMask = other.mMask;
  priority = other.priority;
  entryList = other.entryList;
  listSize = other.listSize;

  // by convention, always return *this
  return *this;
}



//inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(HOTSingleThreadedChildPointer const & other)
//{
//    #ifdef PING_DEBUG9
//  printf("-----------------test2-------------------\n");
//#endif
//  mPointer = other.mPointer;
//  mMask = other.mMask;
//  priority = other.priority;
//  entryList.clear();

// std::vector<entryVector>::const_iterator it;

//  for (it = other.entryList.begin(); it != other.entryList.end(); ++it) {
//    printf("value = %llu\n", (*it).value);
//    entryList.push_back(*it);
//  }



//}


/*
 * New function, add the mMask and entryList
 */
inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(hot::commons::NodeType  nodeAlgorithmType, HOTSingleThreadedNodeBase const *node)
  : mPointer((reinterpret_cast<intptr_t>(node) | static_cast<intptr_t>(nodeAlgorithmType)) << 1) {

}


inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(intptr_t leafValue)
  : mPointer((leafValue << 1) | 1) {
#ifdef PING_DEBUG9
  printf("-----------------test4-------------------\n");
#endif
}


/*
 * New function with a new argument
 * added mask from mMask
 */
inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(intptr_t leafValue, uint64_t mask)
  : mPointer((leafValue << 1) | 1), mMask(mask) {
#ifdef PING_DEBUG9
  printf("-----------------test5-------------------\n");
#endif
}

inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(intptr_t leafValue, uint64_t mask, uint64_t priority, std::array<hot::commons::entryVector, 8> WildcardList)
  : mPointer((leafValue << 1) | 1), mMask(mask), priority(priority), entryList(WildcardList), listSize(0) {
#ifdef PING_DEBUG9
  printf("-----------------test6--------4 overloading-----------\n");
#endif
}

/*
 * New function with a new argument
 * added 3 more arguments
 */
inline HOTSingleThreadedChildPointer::HOTSingleThreadedChildPointer(intptr_t leafValue, uint64_t mask, uint64_t priority, std::array<hot::commons::entryVector, 8> WildcardList, uint8_t listsize)
  : mPointer((leafValue << 1) | 1), mMask(mask), priority(priority), entryList(WildcardList), listSize(listsize) {
#ifdef PING_DEBUG9
  printf("-----------------test7--------5 overloading-----------\n");
#endif
}

/*
 * Need to update the operator for the std::list< >
 * you cannot just use "=" to get the list, which is not the same as the integer or intptr_t
 *
 */

//HOTSingleThreadedChildPointer & HOTSingleThreadedChildPointer::operator=(const HOTSingleThreadedChildPointer &other) {
//#ifdef PING_DEBUG9
//printf("-----------------test7-------------------\n");
//#endif
//  mPointer = other.mPointer;
//  mMask = other.mMask;
//  priority = other.priority;
//  entryList.clear();

//  for (auto it = other.entryList.begin(); it != other.entryList.end(); ++it) {
//    entryList.push_back(*it);
//  }

//  // by convention, always return *this
//  return *this;
//}



inline bool HOTSingleThreadedChildPointer::operator==(HOTSingleThreadedChildPointer const & other) const {
  return (mPointer == other.mPointer && mMask == other.mMask);
}

inline bool HOTSingleThreadedChildPointer::operator!=(HOTSingleThreadedChildPointer const & other) const {
  return (mPointer != other.mPointer || mMask != other.mMask);
}

inline void HOTSingleThreadedChildPointer::free() const {
  executeForSpecificNodeType(false, [&](const auto & node) -> void {
    delete &node;
  });
}

constexpr intptr_t NODE_ALGORITH_TYPE_HELPER_EXTRACTION_MASK = NODE_ALGORITHM_TYPE_EXTRACTION_MASK << 1;
inline hot::commons::NodeType  HOTSingleThreadedChildPointer::getNodeType() const {
  const unsigned int nodeAlgorithmCode = static_cast<unsigned int>(mPointer & NODE_ALGORITH_TYPE_HELPER_EXTRACTION_MASK);
  return static_cast<hot::commons::NodeType >(nodeAlgorithmCode >> 1u);
}

inline HOTSingleThreadedNodeBase* HOTSingleThreadedChildPointer::getNode() const {
  intptr_t const nodePointerValue = (mPointer >> 1) & POINTER_EXTRACTION_MASK;
  return reinterpret_cast<HOTSingleThreadedNodeBase *>(nodePointerValue);
}

inline intptr_t HOTSingleThreadedChildPointer::getTid() const {
  // The the value stored in the pseudo-leaf
  //normally this is undefined behaviour lookup for intrinsic working only on x86 cpus replace with instruction for arithmetic shift

  return mPointer >> 1;
}

inline uint64_t HOTSingleThreadedChildPointer::getMask() const {
  // return the mask value: the wildcard information

  return mMask;
}

/*
 * if the entry is the leaf, the mPointer is already pushed, since it << 1 | 1, so the lsb is 1
 * that is why using this to check if this is a leaf
 */
inline bool HOTSingleThreadedChildPointer::isLeaf() const {
  return mPointer & 1;
}

inline bool HOTSingleThreadedChildPointer::isNode() const {
  return !isLeaf();
}

inline bool HOTSingleThreadedChildPointer::isAValidNode() const {
  return isNode() & (mPointer != reinterpret_cast<intptr_t>(nullptr));
}

inline bool HOTSingleThreadedChildPointer::isUnused() const {
  return (!isLeaf()) & (getNode() == nullptr);
}

inline uint16_t HOTSingleThreadedChildPointer::getHeight() const {
  return isLeaf() ? 0 : getNode()->mHeight;
}

template<typename... Args> inline __attribute__((always_inline)) auto HOTSingleThreadedChildPointer::search(Args... args) const {
  return executeForSpecificNodeType(true,	[&](const auto & node) {
    return node.search(args...);
  });
}

inline unsigned int HOTSingleThreadedChildPointer::getNumberEntries() const {
  return isLeaf() ? 1 : getNode()->getNumberEntries();
}

inline std::set<uint16_t> HOTSingleThreadedChildPointer::getDiscriminativeBits() const {
#ifdef PING_DEBUG3
  std::cout << "HOTSingleThreadedChildPointer::getDiscriminativeBits function ()----" << std::endl;
#endif
  return executeForSpecificNodeType(false, [&](const auto & node) {
    return node.mDiscriminativeBitsRepresentation.getDiscriminativeBits();
  });
}

inline HOTSingleThreadedChildPointer HOTSingleThreadedChildPointer::getSmallestLeafValueInSubtree() const {
  return isLeaf() ? *this : getNode()->getPointers()[0].getSmallestLeafValueInSubtree();
}

inline HOTSingleThreadedChildPointer HOTSingleThreadedChildPointer::getLargestLeafValueInSubtree() const {
  return isLeaf() ? *this : getNode()->getPointers()[getNode()->getNumberEntries() - 1].getLargestLeafValueInSubtree();
}

inline void HOTSingleThreadedChildPointer::deleteSubtree() {
  if(isNode() && getNode() != nullptr) {
    executeForSpecificNodeType(true, [](auto & node) -> void {
      for(HOTSingleThreadedChildPointer & childPointer : node) {
        childPointer.deleteSubtree();
      }
      delete &node;
    });
  }
}

} }

#endif
