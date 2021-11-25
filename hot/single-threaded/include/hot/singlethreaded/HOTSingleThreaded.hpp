#ifndef __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED__
#define __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED__

#include <immintrin.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <set>
#include <map>
#include <numeric>
#include <cstring>
#include <bitset>
#include <list>
#include <iterator>

#include "hot/commons/include/hot/commons/SparsePartialKeys.hpp"
#include <hot/commons/include/hot/commons/Algorithms.hpp>
#include <hot/commons/include/hot/commons/BiNode.hpp>
#include <hot/commons/include/hot/commons/DiscriminativeBit.hpp>
#include <hot/commons/include/hot/commons/InsertInformation.hpp>
#include <hot/commons/include/hot/commons/TwoEntriesNode.hpp>
#include <hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedNodeBase.hpp>

#include <hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedChildPointerInterface.hpp>

#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedInterface.hpp"
#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedNode.hpp"


//Helper Data Structures
#include "HOTSingleThreadedInsertStackEntry.hpp"

#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedIterator.hpp"
#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedDeletionInformation.hpp"

#include "idx/content-helpers/include/idx/contenthelpers/KeyUtilities.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/TidConverters.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/ContentEquals.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/KeyComparator.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/OptionalValue.hpp"




extern hot_trie hotTrie;
extern int branch_index;
extern int key_index;
extern int T_left;
extern int T_right;

extern bool flag_mask; // show if there has wildcard rules needs to be inserted, using my new way
extern uint64_t input_pp;

extern uint64_t wildRule_count;
extern uint32_t wildcard_affected_entry_mask;



namespace hot { namespace singlethreaded {

template<typename ValueType, template <typename> typename KeyExtractor> KeyExtractor<ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::extractKey;
template<typename ValueType, template <typename> typename KeyExtractor>
typename idx::contenthelpers::KeyComparator<typename  HOTSingleThreaded<ValueType, KeyExtractor>::KeyType>::type
HOTSingleThreaded<ValueType, KeyExtractor>::compareKeys;

template<typename ValueType, template <typename> typename KeyExtractor> typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::END_ITERATOR {};

template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor>::HOTSingleThreaded() : mRoot {} {

}

template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor>::HOTSingleThreaded(HOTSingleThreaded && other) {
  //  && usage: && is new in C++11, and it signifies that the function accepts an RValue-Reference -- that is, a reference to an argument that is about to be destroyed.
  mRoot = other.mRoot;
  other.mRoot = {}; //  if it is a empty brace {}, which means use the default initilization values for data members

}

//template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor>::HOTSingleThreaded(HOTSingleThreaded && other) {
//  //  && usage: && is new in C++11, and it signifies that the function accepts an RValue-Reference -- that is, a reference to an argument that is about to be destroyed.
//  mRoot = other.mRoot;
//  /*other.mRoot = {};*/ //  if it is a empty brace {}, which means use the default initilization values for data members
//  other.mRoot = NULL;
//}

//template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor> & HOTSingleThreaded<ValueType, KeyExtractor>::operator=(HOTSingleThreaded && other) {
//  mRoot = other.mRoot;
//  //  other.mRoot = {};
//  other.mRoot = NULL;
//  return *this;
//}

template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor> & HOTSingleThreaded<ValueType, KeyExtractor>::operator=(HOTSingleThreaded && other) {
  mRoot = other.mRoot;
  other.mRoot = {};

  return *this;
}

template<typename ValueType, template <typename> typename KeyExtractor> HOTSingleThreaded<ValueType, KeyExtractor>::~HOTSingleThreaded() {
  mRoot->deleteSubtree();
}

template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::isEmpty() const {
  return !mRoot->isLeaf() & (mRoot->getNode() == nullptr);
}

template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::isRootANode() const {
  return mRoot->isNode() & (mRoot->getNode() != nullptr);
}


template<typename ValueType, template <typename> typename KeyExtractor>inline __attribute__((always_inline)) idx::contenthelpers::OptionalValue<ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::lookup(HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const &key) {
#ifdef PING_DEBUG3
  std::cout << "mTrie.lookup function, key = " << std::bitset<64>(key) << std::endl;
#endif
  auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(key));
#ifdef PING_DEBUG3
  std::cout << "fixedSizeKey size = " << sizeof(fixedSizeKey)/sizeof(uint8_t) << std::endl;
#endif
  uint8_t const* byteKey = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);
#ifdef PING_DEBUG3
  std::cout << "byteKey = " << std::bitset<8>(*byteKey) << std::endl;
#endif
  HOTSingleThreadedChildPointer current;

  if (key_index == 1) {
    mRoot = &mRoot_right;
    current = *mRoot;
  }
  else {
    mRoot = &mRoot_left;
    current = *mRoot;
  }
#ifdef PING_DEBUG3
  std::cout << "-------start search through the hot tree---------" << std::endl;
  std::cout << "---check if current is the leaf---current.isLeaf = " << current.isLeaf() << std::endl;
#endif


  while((!current.isLeaf()) & (current.getNode() != nullptr)) {
#ifdef PING_DEBUG
    printf("-----current = %p\n", current);
#endif
    HOTSingleThreadedChildPointer const * const & currentChildPointer = current.search(byteKey);
    current = *currentChildPointer;
#ifdef PING_DEBUG
    printf("----after------------------current = %p\n", current);
#endif

  }

  return current.isLeaf() ? extractAndMatchLeafValue(current, key, current.mMask) : idx::contenthelpers::OptionalValue<ValueType>();

}

template<typename ValueType, template <typename> typename KeyExtractor>idx::contenthelpers::OptionalValue <ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::extractAndMatchLeafValue( HOTSingleThreadedChildPointer const & current, HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const &key) {
  ValueType const & value = idx::contenthelpers::tidToValue<ValueType>(current.getTid());
  return { idx::contenthelpers::contentEquals(extractKey(value), key), value };
}

/*
 * New function: extractAndMatchLeafValue
 * new argument: mask
 */

template<typename ValueType, template <typename> typename KeyExtractor>idx::contenthelpers::OptionalValue <ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::extractAndMatchLeafValue( HOTSingleThreadedChildPointer const & current, HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const &key, uint64_t mask) {
  ValueType const & value = idx::contenthelpers::tidToValue<ValueType>(current.getTid());
#ifdef PING_DEBUG3
  std::cout << "Value = " << std::bitset<64>(value) << " , key = " << std::bitset<64>(key) << std::endl;
#endif

  if (idx::contenthelpers::contentEquals(extractKey(value), key, mask)) {
    //  find a match
    return { true, value, mask };
  }
  else if (current.listSize > 0) {
#ifdef PING_DEBUG1
//    std::cout << "---------entryList has items------size = ---" << current.listSize << std::endl;
    printf("---------entryList has items------size = ---%i\n", current.listSize);
#endif

//    ValueType const & value1 = idx::contenthelpers::tidToValue<ValueType>(current.entryList[0].value >> 1);
//    return { idx::contenthelpers::contentEquals(extractKey(value1), key, current.entryList[0].mask), value1, current.entryList[0].mask };

  for (int i = 0; i < current.listSize; ++i) {
    ValueType const & value1 = idx::contenthelpers::tidToValue<ValueType>(current.entryList[i].value >> 1);
    bool rst_IsValid = idx::contenthelpers::contentEquals(extractKey(value1), key, current.entryList[i].mask);
    if (rst_IsValid) {
      //  find a match
#ifdef PING_DEBUG9
    std::cout << "--------find a match------entryList index = ---" << i << std::endl;
#endif
      return { true, value1, current.entryList[i].mask };
    }

  }

  return {false, value, mask};

  }
  else {
    return { false, value, mask };
  }

}


template<typename ValueType, template <typename> typename KeyExtractor>inline idx::contenthelpers::OptionalValue<ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::scan(KeyType const &key, size_t numberValues) const {
  const_iterator iterator = lower_bound(key);
  for(size_t i = 0u; i < numberValues && iterator != end(); ++i) {
    ++iterator;
  }
  return iterator == end() ? idx::contenthelpers::OptionalValue<ValueType>({}) : idx::contenthelpers::OptionalValue<ValueType>({ true, *iterator });
}

template<typename ValueType, template <typename> typename KeyExtractor>inline unsigned int HOTSingleThreaded<ValueType, KeyExtractor>::searchForInsert(uint8_t const * keyBytes, std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack) {
#ifdef PING_DEBUG3
  std::cout << "searchForInsert function () starts----- " << std::endl;
  //printf("mRoot = %p, mRoot.mPointer = %p\n", mRoot, mRoot.mPointer);
#endif
  HOTSingleThreadedChildPointer* current = mRoot;
#ifdef PING_DEBUG
  printf("current.value = %p, current.value.mPointer = %p\n", *current, (*current).mPointer);
#endif
  unsigned int currentDepth = 0;
  while(!current->isLeaf()) {
#ifdef PING_DEBUG3
    std::cout << "current isLeaf = " << current->isLeaf() << ", " << "currentDepth: " << currentDepth << std::endl;
#endif
    insertStack[currentDepth].mChildPointer = current;
    current = current->executeForSpecificNodeType(true, [&](auto & node) {
      HOTSingleThreadedInsertStackEntry & currentStackEntry = insertStack[currentDepth];
#ifdef PING_DEBUG3
      printf("init value: currentStackEntry.mSearchResultForInsert.mEntryIndex = %d, currentStackEntry.mSearchResultForInsert.mMostSignificantBitIndex = %d\n", currentStackEntry.mSearchResultForInsert.mEntryIndex, currentStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
#endif
      return node.searchForInsert(currentStackEntry.mSearchResultForInsert, keyBytes);
    });
    ++currentDepth;
  }
#ifdef PING_DEBUG3
  printf("currentDepth = %d\n", currentDepth);
#endif
  insertStack[currentDepth].initLeaf(current);
#ifdef PING_DEBUG3
  printf("====searchForInsert function () ends-----currentDepth = %d\n", currentDepth);
#endif
  return currentDepth;
}

/*
 * need to add prefix rules insertion (wildcard rules 010**)
 * if rule.mask > 0  ====>> for comparison, we just compare the bits without wildcard bit
 * if the non-wildcard DB bits are different, then insert using the normal way
 * else: find the BiNode, then get the affectedSubtree ===>> add this wildcard rule to the entries under the affectedSubtree
 * to do: priority, leaf node ===>> has an array with pair<uint64_t, uint64_t>
 *
 */

template<typename ValueType, template <typename> typename KeyExtractor>inline unsigned int HOTSingleThreaded<ValueType, KeyExtractor>::  searchForInsert(uint8_t const * keyBytes, std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, uint64_t mask) {
#ifdef PING_DEBUG3
  std::cout << "New searchForInsert function () starts----- " << std::endl;
#endif
  HOTSingleThreadedChildPointer* current = mRoot;
#ifdef PING_DEBUG
  printf("current.value = %p, current.value.mPointer = %p\n", *current, (*current).mPointer);
#endif
  unsigned int currentDepth = 0;
  while(!current->isLeaf()) {
#ifdef PING_DEBUG3
    std::cout << "-------current isLeaf = " << current->isLeaf() << ", " << "currentDepth: " << currentDepth << std::endl;
#endif
    insertStack[currentDepth].mChildPointer = current;
    current = current->executeForSpecificNodeType(true, [&](auto & node) {
      HOTSingleThreadedInsertStackEntry & currentStackEntry = insertStack[currentDepth];
#ifdef PING_DEBUG3
      printf("init value: currentStackEntry.mSearchResultForInsert.mEntryIndex = %d, currentStackEntry.mSearchResultForInsert.mMostSignificantBitIndex = %d\n", currentStackEntry.mSearchResultForInsert.mEntryIndex, currentStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
#endif
      return node.searchForInsert(currentStackEntry.mSearchResultForInsert, keyBytes, mask);
    });
    ++currentDepth;
  }
  //  find the leaf node, then compare the rule inserted in the leaf node
#ifdef PING_DEBUG3
  printf("----find the leafNode-------currentDepth = %d\n", currentDepth);
#endif
  insertStack[currentDepth].initLeaf(current);
#ifdef PING_DEBUG3
  printf("==New==searchForInsert function () ends-----currentDepth = %d\n", currentDepth);
#endif
  return currentDepth;
}




template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::remove(KeyType const & key) {
#ifdef PING_DEBUG
  printf("HOTSingleThreaded::remove function () ------begins----\n");
  printf("key = %d\n", key);
#endif
  auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(key));
  uint8_t const* keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);
  bool wasContained = false;
#ifdef PING_DEBUG
  uint8_t *val = (uint8_t*)&keyBytes;
  std::cout << "keyBytes = ";
  printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
  if(isRootANode()) {
#ifdef PING_DEBUG
    printf("isRootANode = %d\n", isRootANode());
#endif
    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
    unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
#ifdef PING_DEBUG
    printf("----leafDepth = %d\n", leafDepth);
#endif
    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
    KeyType const & existingKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));
#ifdef PING_DEBUG
    printf("existingKey = %d\n", existingKey);
#endif
    wasContained = idx::contenthelpers::contentEquals(existingKey, key);
    if(wasContained) {
      removeWithStack(insertStack, leafDepth - 1);
    }
  } else if(mRoot->isLeaf() && hasTheSameKey(mRoot->getTid(), key)) {
#ifdef PING_DEBUG
    printf("mRoot is the leaf and has the same key\n");
#endif
    *mRoot = HOTSingleThreadedChildPointer();
    wasContained = true;
  }
#ifdef PING_DEBUG
  printf("HOTSingleThreaded::remove function () ------ends--return wasContained = %d\n", wasContained);
#endif
  return wasContained;
};

template<typename ValueType,  template <typename> typename KeyExtractor>
void
HOTSingleThreaded<ValueType, KeyExtractor>::removeWithStack(std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack, unsigned int currentDepth) {
#ifdef PING_DEBUG
  printf("----removeWithStack function () begins----\n");
#endif
  removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(searchStack, currentDepth, determineDeletionInformation(searchStack, currentDepth), [](HOTSingleThreadedChildPointer const & newNode, size_t /* offset */){
    return newNode;
  });
#ifdef PING_DEBUG
  printf("======----removeWithStack function () ends----\n");
#endif
}

template<typename ValueType,  template <typename> typename KeyExtractor>
void HOTSingleThreaded<ValueType, KeyExtractor>::removeRecurseUp(std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack, unsigned int currentDepth,  HOTSingleThreadedDeletionInformation const & deletionInformation, HOTSingleThreadedChildPointer const & replacement) {
  if(deletionInformation.getContainingNode().getNumberEntries() == 2) {
    HOTSingleThreadedChildPointer previous = *searchStack[currentDepth].mChildPointer;
    *searchStack[currentDepth].mChildPointer = replacement;
    previous.free();
  } else {
    removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(searchStack, currentDepth, deletionInformation, [&](HOTSingleThreadedChildPointer const & newNode, size_t offset){
      newNode.getNode()->getPointers()[offset + deletionInformation.getIndexOfEntryToReplace()] = replacement;
      return newNode;
    });
  }
}

template<typename ValueType, template <typename> typename KeyExtractor> template<typename Operation>
void HOTSingleThreaded<ValueType, KeyExtractor>::removeAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(
    std::array<HOTSingleThreadedInsertStackEntry, 64> const &searchStack, unsigned int currentDepth, HOTSingleThreadedDeletionInformation const & deletionInformation, Operation const & operation
    )
{
  HOTSingleThreadedChildPointer* current = searchStack[currentDepth].mChildPointer;
  bool isRoot = currentDepth == 0;
  if(isRoot) {
    removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current, deletionInformation, operation);
  } else {
    unsigned int parentDepth = currentDepth - 1;
    HOTSingleThreadedDeletionInformation const & parentDeletionInformation = determineDeletionInformation(searchStack, parentDepth);
    bool hasDirectNeighbour = parentDeletionInformation.hasDirectNeighbour();
    if(hasDirectNeighbour) {
      HOTSingleThreadedChildPointer* potentialDirectNeighbour = parentDeletionInformation.getDirectNeighbourIfAvailable();
      if((potentialDirectNeighbour->getHeight() == current->getHeight())) {
        size_t totalNumberEntries = potentialDirectNeighbour->getNumberEntries() + current->getNumberEntries() - 1;
        if(totalNumberEntries <= MAXIMUM_NUMBER_NODE_ENTRIES) {
          HOTSingleThreadedNodeBase *parentNode = searchStack[parentDepth].mChildPointer->getNode();
          HOTSingleThreadedChildPointer left = parentNode->getPointers()[parentDeletionInformation.getAffectedBiNode().mLeft.mFirstIndexInRange];
          HOTSingleThreadedChildPointer right = parentNode->getPointers()[parentDeletionInformation.getAffectedBiNode().mRight.mFirstIndexInRange];
          HOTSingleThreadedChildPointer mergedNode = operation(
                mergeNodesAndRemoveEntryIfPossible(
                  parentDeletionInformation.getAffectedBiNode().mDiscriminativeBitIndex, left, right,
                  deletionInformation, parentDeletionInformation.getDiscriminativeBitValueForEntry()
                  ),
                //offset in case the deleted entry is in the right side
                left.getNumberEntries() * parentDeletionInformation.getDiscriminativeBitValueForEntry()
                );
          assert(!mergedNode.isUnused() && mergedNode.isNode());
          removeRecurseUp(searchStack, parentDepth, parentDeletionInformation, mergedNode);
          left.free();
          right.free();
        } else {
          removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current, deletionInformation, operation);
        }
      } else if((potentialDirectNeighbour->getHeight() < current->getHeight())) {
        //this is required in case for the creation of this tree a node split happened, resulting in a link to a leaf or a node of smaller height
        //move directNeighbour into current and remove
        //removeAndAdd
        hot::commons::DiscriminativeBit keyInformation(parentDeletionInformation.getAffectedBiNode().mDiscriminativeBitIndex, !parentDeletionInformation.getDiscriminativeBitValueForEntry());
        HOTSingleThreadedChildPointer previousNode = *current;
        HOTSingleThreadedChildPointer newNode = operation(current->executeForSpecificNodeType(false, [&](auto const & currentNode) {
          return currentNode.removeAndAddEntry(deletionInformation, keyInformation, *potentialDirectNeighbour);
        }), parentDeletionInformation.getDiscriminativeBitValueForEntry());
        removeRecurseUp(searchStack, parentDepth, parentDeletionInformation, newNode);
        previousNode.free();
      } else {
        removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current, deletionInformation, operation);
      }
    } else {
      removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(current, deletionInformation, operation);
    }
  }
};

template<typename ValueType, template <typename> typename KeyExtractor> template<typename Operation>
void HOTSingleThreaded<ValueType, KeyExtractor>::removeEntryAndExecuteOperationOnNewNodeBeforeIntegrationIntoTreeStructure(
    HOTSingleThreadedChildPointer* const currentNodePointer, HOTSingleThreadedDeletionInformation const & deletionInformation, Operation const & operation
    )
{
  HOTSingleThreadedChildPointer previous = *currentNodePointer;
  *currentNodePointer = operation(
        currentNodePointer->executeForSpecificNodeType(false, [&](auto const & currentNode){
    return currentNode.removeEntry(deletionInformation);
  }),
        0
        );
  previous.free();
};


template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::insert(ValueType const & value) {
  bool inserted = true;
#ifdef PING_DEBUG3
  printf("-------insert function ()-----starts\n");
  std::cout << "======insert value ====== " << std::bitset<64>(value) << std::endl;
  printf("branch_index = %d, T_right = %d, T_left = %d\n", branch_index, T_right, T_left);
  //printf("mRoot = %p, mRoot_right = %p, mRoot_left = %p\n", mRoot, mRoot_right, mRoot_left);
#endif
  auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(value)));
#ifdef PING_DEBUG3
  uint8_t *val = (uint8_t*)&fixedSizeKey;
  std::cout << "fixedSizeKey = ";
  printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
  //  printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif

  uint8_t const* keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);
#ifdef PING_DEBUG3
  std::cout << "-------------------keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
  printf("hotTrie.root->children[%d] = %p\n", branch_index, hotTrie.root->children[branch_index]);
#endif

  if(branch_index == 1 && T_right == 1) {
    //  the right branch trie is empty
#ifdef PING_DEBUG3
    std::cout << "*****right branch---------mRoot is NULL-------" << std::endl;
    std::cout << "mRoot is not leaf, getNode is null: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value   << ", binary form: " << std::bitset<64>(value) << std::endl;
#endif

    mRoot_right = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(value));
    *(hotTrie.root->children[branch_index]) = reinterpret_cast<intptr_t>(&mRoot_right);
    mRoot = &mRoot_right;
  }

  if(branch_index == 0 && T_left == 1) {
    //  the left branch trie is empty
#ifdef PING_DEBUG3
    std::cout << "*****left branch---------mRoot is NULL-------" << std::endl;
    //std::cout << "mRoot is not leaf, getNode is null: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value  << ", binary form: " << std::bitset<64>(value) << std::endl;
#endif
    mRoot_left = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(value));
    *(hotTrie.root->children[branch_index]) = reinterpret_cast<intptr_t>(&mRoot_left);
    mRoot = &mRoot_left;

  }

  if(branch_index == 1 && T_right == 0) {
    //  go to the right branch trie, and it is not empty

#ifdef PING_DEBUG3
    printf("-------branch_index == 1 && T_right == 0-----------------\n");
    //printf("mRoot = %p, mRoot_right = %p, mRoot_left = %p\n", mRoot, mRoot_right, mRoot_left);
    //printf("mRoot.mPointer = %p------mRoot_right.mPointer = %p---------\n", mRoot.mPointer, mRoot_right.mPointer);
#endif

    mRoot = &mRoot_right;

    if(isRootANode()) {
      //  mRoot is not the leaf, getNode() != NULL
#ifdef PING_DEBUG3
      std::cout << "===mRoot (right branch) is not the leaf, and the node is not NULL===Root is a node====== " << std::endl;
#endif
      std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
      unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
      intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
#ifdef PING_DEBUG3
      std::cout << "tid =         " << std::bitset<64>(tid) << ", leafDepth = " << leafDepth << std::endl;
#endif
      KeyType const & existingKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));
#ifdef PING_DEBUG3
      std::cout << "existingKey = " << std::bitset<64>(*(&existingKey)) << ", keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
#endif
      inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value);
    } else if(mRoot->isLeaf()) {
      //  mRoot is the leaf
#ifdef PING_DEBUG3
      std::cout << "mRoot (right branch) is the leaf: " << mRoot->isLeaf() << std::endl;
      std::cout << "value = " << value << ", binary format = " << std::bitset<64>(value) << std::endl;
#endif
      HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(value));
#ifdef PING_DEBUG
      std::cout << "valueToInsert = ";
      uint8_t *v_mPointer = (uint8_t*)&valueToInsert.mPointer;
      printf("%p %p %p %p %p %p %p %p\n",v_mPointer[0],v_mPointer[1],v_mPointer[2],v_mPointer[3],v_mPointer[4],v_mPointer[5], v_mPointer[6], v_mPointer[7]);
#endif

      ValueType const & currentLeafValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());  // getTid() is used to change back to the real value, since it made change by << 1 | 1 when create the node pointer.
#ifdef PING_DEBUG3
      std::cout << "currentLeafValue = " << currentLeafValue << ", binary format = " << std::bitset<64>(currentLeafValue) << std::endl;
#endif
      auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(currentLeafValue)));
#ifdef PING_DEBUG3
      uint8_t *val = (uint8_t*)&existingFixedSizeKey;
      std::cout << "existingFixedSizeKey = ";
      printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
      //      printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
      uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
#ifdef PING_DEBUG3
      std::cout << "existingKeyBytes = " << std::bitset<64>(*existingKeyBytes) << std::endl;
#endif
      inserted = hot::commons::executeForDiffingKeys(existingKeyBytes, keyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    #ifdef PING_DEBUG3
          std::cout << "if key are different, is true, inserted = " << inserted << std::endl;
          std::cout << "DiscriminativeBit information: " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", " <<
                       significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << std::endl;
          std::cout << "mRoot.Height = " << mRoot->getHeight() << std::endl;
    #endif
          hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(significantKeyInformation, *mRoot, valueToInsert);
    #ifdef PING_DEBUG
          std::cout << "binaryNode.mDiscriminativeBitIndex = " << binaryNode.mDiscriminativeBitIndex << ", binaryNode.mHeight = " << binaryNode.mHeight << std::endl;
          printf("mLeft.address = %p\n", &binaryNode.mLeft);
          uint8_t *left_val = (uint8_t*)&binaryNode.mLeft;
          uint8_t *right_val = (uint8_t*)&binaryNode.mRight;
          //printf("mLeft.value = %p %p %p %p %p %p %p %p\n",left_val[0],left_val[1],left_val[2],left_val[3],left_val[4],left_val[5], left_val[6], left_val[7]);
          printf("mRight.address = %p\n", &binaryNode.mRight);
          //printf("mRight.value = %p %p %p %p %p %p %p %p\n",right_val[0],right_val[1],right_val[2],right_val[3],right_val[4],right_val[5], right_val[6], right_val[7]);
    #endif
          *mRoot = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(binaryNode)->toChildPointer();

    #ifdef PING_DEBUG3
          std::cout << "Print out the Discriminative bit for the tree:" << std::endl;
          std::set<uint16_t> ping_set = mRoot->getDiscriminativeBits();
          std::cout << "DiscriminativeBits set size = " << ping_set.size() << std::endl;
          for (std::set<uint16_t>::iterator it = ping_set.begin(); it != ping_set.end(); it++) {
        std::cout << "DB: bit position: " << *it << std::endl;
      }
#endif

    });

  } else {
#ifdef PING_DEBUG3
    std::cout << "mRoot *****(right branch)***** is NULL-------" << std::endl;
    //std::cout << "mRoot is not leaf, getNode is null: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value << std::endl;
#endif

  }

}

if(branch_index == 0 && T_left == 0) {
  //  go to the left branch trie, and it is not empty

#ifdef PING_DEBUG3
  printf("-------branch_index == 0 && T_left == 0-----------------\n");
  //printf("mRoot = %p, mRoot_right = %p, mRoot_left = %p\n", mRoot, mRoot_right, mRoot_left);
  //printf("mRoot.mPointer = %p------mRoot_left.mPointer = %p---------\n", mRoot.mPointer, mRoot_left.mPointer);
#endif

  mRoot = &mRoot_left;

#ifdef PING_DEBUG3
  std::cout << "=======left branch is not empty==============" << std::endl;
#endif

  if(isRootANode()) {
    //  mRoot is not the leaf, getNode() != NULL
#ifdef PING_DEBUG3
    std::cout << "===mRoot (left branch) is not the leaf, and the node is not NULL===Root is a node====== " << std::endl;
#endif
    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
    unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
#ifdef PING_DEBUG3
    std::cout << "tid =         " << std::bitset<64>(tid) << ", leafDepth = " << leafDepth << std::endl;
#endif
    KeyType const & existingKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));
#ifdef PING_DEBUG3
    std::cout << "existingKey = " << std::bitset<64>(*(&existingKey)) << ", keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
#endif
    inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value);
  } else if(mRoot->isLeaf()) {
    //  mRoot is the leaf
#ifdef PING_DEBUG3
    std::cout << "mRoot (left branch) is the leaf: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value << ", binary format = " << std::bitset<64>(value) << std::endl;
#endif
    HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(value));
#ifdef PING_DEBUG
    std::cout << "valueToInsert = ";
    uint8_t *v_mPointer = (uint8_t*)&valueToInsert.mPointer;
    printf("%p %p %p %p %p %p %p %p\n",v_mPointer[0],v_mPointer[1],v_mPointer[2],v_mPointer[3],v_mPointer[4],v_mPointer[5], v_mPointer[6], v_mPointer[7]);
#endif
    ValueType const & currentLeafValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());  // getTid() is used to change back to the real value, since it made change by << 1 | 1 when create the node pointer.
#ifdef PING_DEBUG3
    std::cout << "currentLeafValue = " << currentLeafValue << ", binary format = " << std::bitset<64>(currentLeafValue) << std::endl;
#endif

    auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(currentLeafValue)));
#ifdef PING_DEBUG3
    uint8_t *val = (uint8_t*)&existingFixedSizeKey;
    std::cout << "existingFixedSizeKey = ";
    printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
    //    printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
    uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
#ifdef PING_DEBUG3
    std::cout << "existingKeyBytes = " << std::bitset<64>(*existingKeyBytes) << std::endl;
#endif
    inserted = hot::commons::executeForDiffingKeys(existingKeyBytes, keyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    #ifdef PING_DEBUG3
        std::cout << "if key are different, is true, inserted = " << inserted << std::endl;
        std::cout << "DiscriminativeBit information: " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", " <<
                     significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << std::endl;
        //std::cout << "mRoot.Height = " << mRoot->getHeight() << std::endl;
    #endif

        hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(significantKeyInformation, *mRoot, valueToInsert);
    #ifdef PING_DEBUG
        std::cout << "binaryNode.mDiscriminativeBitIndex = " << binaryNode.mDiscriminativeBitIndex << ", binaryNode.mHeight = " << binaryNode.mHeight << std::endl;
        printf("mLeft.address = %p\n", &binaryNode.mLeft);
        uint8_t *left_val = (uint8_t*)&binaryNode.mLeft;
        uint8_t *right_val = (uint8_t*)&binaryNode.mRight;
        //printf("mLeft.value = %p %p %p %p %p %p %p %p\n",left_val[0],left_val[1],left_val[2],left_val[3],left_val[4],left_val[5], left_val[6], left_val[7]);
        printf("mRight.address = %p\n", &binaryNode.mRight);
        //printf("mRight.value = %p %p %p %p %p %p %p %p\n",right_val[0],right_val[1],right_val[2],right_val[3],right_val[4],right_val[5], right_val[6], right_val[7]);
    #endif
        *mRoot = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(binaryNode)->toChildPointer();

    #ifdef PING_DEBUG
        printf("*******************check here ---gagin*****---again----*******************\n");
        std::cout << "Print out the Discriminative bit for the tree:" << std::endl;
        std::set<uint16_t> ping_set = mRoot->getDiscriminativeBits();
        std::cout << "DiscriminativeBits set size = " << ping_set.size() << std::endl;
        for (std::set<uint16_t>::iterator it = ping_set.begin(); it != ping_set.end(); it++) {
      std::cout << "DB: bit position: " << *it << std::endl;
    }
#endif
  });

} else {
printf("9999999999999999999999999999999==should never come here \n");

}
              }

#ifdef PING_DEBUG3
printf("=======insert function () ends-----inserted = %d*********======================\n", inserted);
#endif

return inserted;
}

/*
 * new insert function for wildcard matching
 * added another parameter: uint64_t mask
 */

template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::insert(ValueType & value, uint64_t mask) {
  bool inserted = true;
#ifdef PING_DEBUG3
  printf("-------New insert function ()----includes mask part------starts\n");
  std::cout << "======insert value ====== " << std::bitset<64>(value) << ", mask = " << std::bitset<64>(mask) << std::endl;
#endif
  auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(value)));

#ifdef PING_DEBUG3
  uint8_t *val = (uint8_t*)&fixedSizeKey;
  std::cout << "fixedSizeKey = " << std::bitset<64>(*val) << std::endl;
#endif


  uint8_t const * keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);
#ifdef PING_DEBUG3
  std::cout << "keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
#endif

  uint64_t priority = input_pp;
#ifdef PING_DEBUG3
  printf("------------------------Insert rule priority = %lu------\n", priority);
#endif
  std::array<hot::commons::entryVector, 8> WildcardList;


  if(branch_index == 1 && T_right == 1) {
    //  the right branch trie is empty
    mRoot_right = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(value), mask, priority, WildcardList);
    *(hotTrie.root->children[branch_index]) = reinterpret_cast<intptr_t>(&mRoot_right);
    mRoot = &mRoot_right;
  }



  if(branch_index == 0 && T_left == 1) {
    //  the left branch trie is empty
    mRoot_left = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(value), mask, priority, WildcardList);
#ifdef PING_DEBUG
    printf("---after operation-----\n");
    std::cout << "*****--------mRoot_left.mPointer = " << mRoot_left.mPointer << ", mRoot_left.mMask =-------"  << mRoot_left.mMask << std::endl;
#endif
    *(hotTrie.root->children[branch_index]) = reinterpret_cast<intptr_t>(&mRoot_left);
    mRoot = &mRoot_left;

  }

  if(branch_index == 1 && T_right == 0) {
    //  go to the right branch trie, and it is not empty

    mRoot = &mRoot_right;

    if(isRootANode()) {
      //  mRoot is not the leaf, getNode() != NULL
#ifdef PING_DEBUG3
      std::cout << "===mRoot (right branch) is not the leaf, and the node is not NULL===Root is a node====== " << std::endl;
#endif
      std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
      //      unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
      unsigned int leafDepth = searchForInsert(keyBytes, insertStack, mask);
      intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();

      uint64_t matchedMask = insertStack[leafDepth].mChildPointer->mMask;

  #ifdef PING_DEBUG3
      std::cout << "======================derek..........." << std::endl;
      std::cout << "matchedMask = " << std::bitset<64>(matchedMask) << std::endl;
  #endif

  #ifdef PING_DEBUG3
      std::cout << "tid         = " << std::bitset<64>(tid) << ", leafDepth = " << leafDepth << std::endl;
  #endif
      KeyType const & existingKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));

      // need to get the existingMask too, since the matchedIndex might has "*" too, need to avoid these wildcard too
  #ifdef PING_DEBUG3
      std::cout << "existingKey = " << std::bitset<64>(*(&existingKey)) << ", keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
      std::cout << "value       = " << std::bitset<64>(value) << std::endl;
  #endif

      //inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value, mask, priority, WildcardList);
      inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, matchedMask, keyBytes, value, mask, priority, WildcardList);

      //inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value);
    } else if(mRoot->isLeaf()) {
      //  mRoot is the leaf
#ifdef PING_DEBUG3
      std::cout << "mRoot (right branch) is the leaf: " << mRoot->isLeaf() << std::endl;
      std::cout << "value = " << value << ", binary format = " << std::bitset<64>(value) << std::endl;
#endif
      HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(value), mask, priority, WildcardList);
#ifdef PING_DEBUG3
      std::cout << "valueToInsert.mMask = " << valueToInsert.mMask << std::endl;
#endif


      ValueType const & currentLeafValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());  // getTid() is used to change back to the real value, since it made change by << 1 | 1 when create the node pointer.
#ifdef PING_DEBUG3
      std::cout << "currentLeafValue = " << currentLeafValue << ", binary format = " << std::bitset<64>(currentLeafValue) << std::endl;
#endif
      auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(currentLeafValue)));
#ifdef PING_DEBUG3
      uint8_t *val = (uint8_t*)&existingFixedSizeKey;
      std::cout << "existingFixedSizeKey = ";
      printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
      //      printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
      uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
#ifdef PING_DEBUG3
      std::cout << "existingKeyBytes = " << std::bitset<64>(*existingKeyBytes) << std::endl;
#endif
      inserted = hot::commons::executeForDiffingKeys(mask, existingKeyBytes, keyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    #ifdef PING_DEBUG3
          std::cout << "if key are different, is true, inserted = " << inserted << std::endl;
          std::cout << "DiscriminativeBit information: " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", " <<
                       significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << std::endl;
          //std::cout << "mRoot.Height = " << mRoot->getHeight() << std::endl;
    #endif
          hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(significantKeyInformation, *mRoot, valueToInsert);
    #ifdef PING_DEBUG
          std::cout << "binaryNode.mDiscriminativeBitIndex = " << binaryNode.mDiscriminativeBitIndex << ", binaryNode.mHeight = " << binaryNode.mHeight << std::endl;
          printf("mLeft.address = %p\n", &binaryNode.mLeft);
          uint8_t *left_val = (uint8_t*)&binaryNode.mLeft;
          uint8_t *right_val = (uint8_t*)&binaryNode.mRight;
          //printf("mLeft.value = %p %p %p %p %p %p %p %p\n",left_val[0],left_val[1],left_val[2],left_val[3],left_val[4],left_val[5], left_val[6], left_val[7]);
          printf("mRight.address = %p\n", &binaryNode.mRight);
          //printf("mRight.value = %p %p %p %p %p %p %p %p\n",right_val[0],right_val[1],right_val[2],right_val[3],right_val[4],right_val[5], right_val[6], right_val[7]);
    #endif
          *mRoot = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(binaryNode)->toChildPointer();

    #ifdef PING_DEBUG
          std::cout << "Print out the Discriminative bit for the tree:" << std::endl;
          std::set<uint16_t> ping_set = mRoot->getDiscriminativeBits();
          std::cout << "DiscriminativeBits set size = " << ping_set.size() << std::endl;
          for (std::set<uint16_t>::iterator it = ping_set.begin(); it != ping_set.end(); it++) {
        std::cout << "DB: bit position: " << *it << std::endl;
      }
#endif

    });

  } else {
#ifdef PING_DEBUG3
    std::cout << "mRoot *****(right branch)***** is NULL-------" << std::endl;
    std::cout << "mRoot is not leaf, getNode is null: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value << std::endl;
#endif

  }

}

if(branch_index == 0 && T_left == 0) {
  //  go to the left branch trie, and it is not empty

  mRoot = &mRoot_left;

#ifdef PING_DEBUG3
  std::cout << "=======left branch is not empty==============" << std::endl;
#endif

  if(isRootANode()) {
    //  mRoot is not the leaf, getNode() != NULL
#ifdef PING_DEBUG3
    std::cout << "===mRoot (left branch) is not the leaf, and the node is not NULL===Root is a node====== " << std::endl;
#endif
    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
    unsigned int leafDepth = searchForInsert(keyBytes, insertStack, mask);
    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
    uint64_t matchedMask = insertStack[leafDepth].mChildPointer->mMask;

#ifdef PING_DEBUG3
    std::cout << "======================derek..........." << std::endl;
    std::cout << "matchedMask = " << std::bitset<64>(matchedMask) << std::endl;
#endif

#ifdef PING_DEBUG3
    std::cout << "tid         = " << std::bitset<64>(tid) << ", leafDepth = " << leafDepth << std::endl;
#endif
    KeyType const & existingKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));

    // need to get the existingMask too, since the matchedIndex might has "*" too, need to avoid these wildcard too
#ifdef PING_DEBUG3
    std::cout << "existingKey = " << std::bitset<64>(*(&existingKey)) << ", keyBytes = " << std::bitset<64>(*keyBytes) << std::endl;
    std::cout << "value       = " << std::bitset<64>(value) << std::endl;
#endif

    //inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, keyBytes, value, mask, priority, WildcardList);
    inserted = insertWithInsertStack(insertStack, leafDepth, existingKey, matchedMask, keyBytes, value, mask, priority, WildcardList);

  } else if(mRoot->isLeaf()) {
    //  mRoot is the leaf
#ifdef PING_DEBUG3
    std::cout << "mRoot (left branch) is the leaf: " << mRoot->isLeaf() << std::endl;
    std::cout << "value = " << value << ", binary format = " << std::bitset<64>(value) << std::endl;
#endif
    HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(value), mask, priority, WildcardList);

#ifdef PING_DEBUG3
    std::cout << "valueToInsert.mPointer = " << valueToInsert.mPointer << ", valueToInsert.mMask = " << valueToInsert.mMask << std::endl;
#endif

    ValueType const & currentLeafValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());  // getTid() is used to change back to the real value, since it made change by << 1 | 1 when create the node pointer.
#ifdef PING_DEBUG3
    std::cout << "currentLeafValue = " << currentLeafValue << ", binary format = " << std::bitset<64>(currentLeafValue) << std::endl;
#endif

    auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(currentLeafValue)));
    uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);

    inserted = hot::commons::executeForDiffingKeys(mask, existingKeyBytes, keyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    #ifdef PING_DEBUG3
        std::cout << "if key are different, is true, inserted = " << inserted << std::endl;
        std::cout << "DiscriminativeBit information: " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", " <<
                     significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << std::endl;
    #endif

        hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(significantKeyInformation, *mRoot, valueToInsert);

        *mRoot = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(binaryNode)->toChildPointer();

  });

  } else {
    printf("9999999999999999999999999999999==should never come here \n");

  }
}

#ifdef PING_DEBUG3
printf("=======insert function () ends-----inserted = %d*********======================\n", inserted);
printf("mRoot_left.mMask = %lu, mRoot_right.mMask = %lu\n", mRoot_left.mMask, mRoot_right.mMask);
#endif

return inserted;
}



template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::insertWithInsertStack(
    std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack, unsigned int leafDepth, KeyType const &existingKey,
    uint8_t const *newKeyBytes, ValueType const &newValue) {
#ifdef PING_DEBUG3
  std::cout << "insertWithInsertStack function () begin: " << std::endl;
#endif
  auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(existingKey));
#ifdef PING_DEBUG3
  uint64_t *val = (uint64_t*)&existingFixedSizeKey;
  std::cout << "existingFixedSizeKey = " << std::bitset<64>(*val) << std::endl;
#endif
  uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
  return hot::commons::executeForDiffingKeys(existingKeyBytes, newKeyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    unsigned int insertDepth = 0;
    //searches for the node to insert the new value into.
    //Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
    //this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
#ifdef PING_DEBUG3
    std::cout << "significantKeyInformation.mAbsoluteBitIndex = " << significantKeyInformation.mAbsoluteBitIndex << std::endl;
    std::cout << "insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex = " << insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex << std::endl;
#endif
    while(significantKeyInformation.mAbsoluteBitIndex > insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex) {
      ++insertDepth;
    }
#ifdef PING_DEBUG3
    printf("insertDepth = %d,  leafDepth = %d\n", insertDepth, leafDepth);
#endif
    //this is ensured because mMostSignificantDiscriminativeBitIndex is set to MAX_INT16 for the leaf entry
    assert(insertDepth < leafDepth);

    HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(newValue));
#ifdef PING_DEBUG3
    std::cout << "significantKeyInformation = { " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", "
              << significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << "}" << std::endl;
#endif
    insertNewValueIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);
  });
#ifdef PING_DEBUG3
  std::cout << "insertWithInsertStack function () end------ " << std::endl;
#endif
}

/*
 * added new function insertWithInsertStack with new argument "mask"
 */

template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::insertWithInsertStack(
    std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack, unsigned int leafDepth, KeyType const &existingKey,
    uint8_t const *newKeyBytes, ValueType const &newValue, uint64_t mask, uint64_t priority, std::array<hot::commons::entryVector, 8> WildcardList) {
#ifdef PING_DEBUG3
  std::cout << "------New insertWithInsertStack function () ---with mask argument------begin: " << std::endl;
  printf("---leafDepth = %d\n", leafDepth);
#endif
  auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(existingKey));
#ifdef PING_DEBUG3
  uint64_t *val = (uint64_t*)&existingFixedSizeKey;
  std::cout << "existingFixedSizeKey = " << std::bitset<64>(*val) << std::endl;
  std::cout << "newValue             = " << std::bitset<64>(newValue) << std::endl;
#endif
  uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
  return hot::commons::executeForDiffingKeys(mask, existingKeyBytes, newKeyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    unsigned int insertDepth = 0;
    /*searches for the node to insert the new value into.
      Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
      this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
    */

#ifdef PING_DEBUG3
    printf("---start while loop:\n");
    std::cout << "insertDepth = " << insertDepth << std::endl;
    std::cout << "significantKeyInformation.mAbsoluteBitIndex = " << significantKeyInformation.mAbsoluteBitIndex << std::endl;
    std::cout << "insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex = " << insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex << std::endl;
#endif

    while(significantKeyInformation.mAbsoluteBitIndex > insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex) {
#ifdef PING_DEBUG3
      printf("---during while loop:\n");
      std::cout << "insertDepth = " << insertDepth << std::endl;
#endif
      ++insertDepth;
    }
#ifdef PING_DEBUG3
    printf("After while loop: insertDepth = %d,  leafDepth = %d\n", insertDepth, leafDepth);
#endif
    //this is ensured because mMostSignificantDiscriminativeBitIndex is set to MAX_INT16 for the leaf entry
    assert(insertDepth < leafDepth);

    HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(newValue), mask, priority, WildcardList);
#ifdef PING_DEBUG3
    std::cout << "---valueToInsert.mPointer = ---" << std::bitset<64>(valueToInsert.mPointer) << std::endl;
    printf("--------------valueToInsert.mMask = %lu--------valueToInsert.entryList.size = %zu\n", valueToInsert.mMask, valueToInsert.entryList.size());
#endif
#ifdef PING_DEBUG3
    std::cout << "significantKeyInformation = { " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", "
              << significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << "}" << std::endl;
#endif

    /* Need to set a branch here, if flag_mask == true, means the non-wildcard part are the same
     * we sill need to insert the wildcard rules, but need to use a different function
     * insertNewWildcardRuleIntoNode( )
     *
     */
    if (flag_mask) {
      insertNewWildcardRuleIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);

    } else {
      insertNewValueIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);
    }

    //    insertNewValueIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);
#ifdef PING_DEBUG3
    std::cout << "New.......insertWithInsertStack function () end------ " << std::endl;
#endif
  });

}

/*
 * Added one argument: matchedMask, since the matchedMask might has wildcard "*" too
 *
 */

template<typename ValueType, template <typename> typename KeyExtractor> inline bool HOTSingleThreaded<ValueType, KeyExtractor>::insertWithInsertStack(
    std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack, unsigned int leafDepth, KeyType const &existingKey, uint64_t matchedMask,
    uint8_t const *newKeyBytes, ValueType const &newValue, uint64_t mask, uint64_t priority, std::array<hot::commons::entryVector, 8> WildcardList) {
#ifdef PING_DEBUG3
  std::cout << "---New---New insertWithInsertStack function () ---with mask argument------begin: " << std::endl;
  printf("---leafDepth = %d\n", leafDepth);
#endif
  auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(existingKey));
#ifdef PING_DEBUG3
  uint64_t *val = (uint64_t*)&existingFixedSizeKey;
  std::cout << "existingFixedSizeKey = " << std::bitset<64>(*val) << std::endl;
  std::cout << "newValue             = " << std::bitset<64>(newValue) << std::endl;
#endif
  uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);
  return hot::commons::executeForDiffingKeys(mask, existingKeyBytes, matchedMask, newKeyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
    unsigned int insertDepth = 0;
    /*searches for the node to insert the new value into.
      Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
      this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
    */

#ifdef PING_DEBUG3
    printf("---start while loop:\n");
    std::cout << "insertDepth = " << insertDepth << std::endl;
    std::cout << "significantKeyInformation.mAbsoluteBitIndex = " << significantKeyInformation.mAbsoluteBitIndex << std::endl;
    std::cout << "insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex = " << insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex << std::endl;
#endif

    while(significantKeyInformation.mAbsoluteBitIndex > insertStack[insertDepth+1].mSearchResultForInsert.mMostSignificantBitIndex) {
#ifdef PING_DEBUG3
      printf("---during while loop:\n");
      std::cout << "insertDepth = " << insertDepth << std::endl;
#endif
      ++insertDepth;
    }
#ifdef PING_DEBUG3
    printf("After while loop: insertDepth = %d,  leafDepth = %d\n", insertDepth, leafDepth);
#endif
    //this is ensured because mMostSignificantDiscriminativeBitIndex is set to MAX_INT16 for the leaf entry
    assert(insertDepth < leafDepth);

    HOTSingleThreadedChildPointer valueToInsert(idx::contenthelpers::valueToTid(newValue), mask, priority, WildcardList);
#ifdef PING_DEBUG3
    std::cout << "---valueToInsert.mPointer = ---" << std::bitset<64>(valueToInsert.mPointer) << std::endl;
    printf("--------------valueToInsert.mMask = %lu--------valueToInsert.entryList.size = %zu\n", valueToInsert.mMask, valueToInsert.entryList.size());
#endif
#ifdef PING_DEBUG3
    std::cout << "significantKeyInformation = { " << significantKeyInformation.mByteIndex << ", " << significantKeyInformation.mByteRelativeBitIndex << ", "
              << significantKeyInformation.mAbsoluteBitIndex << ", " << significantKeyInformation.mValue << "}" << std::endl;
#endif

    /* Need to set a branch here, if flag_mask == true, means the non-wildcard part are the same
     * we sill need to insert the wildcard rules, but need to use a different function
     * insertNewWildcardRuleIntoNode( )
     *
     */
    if (flag_mask) {
      insertNewWildcardRuleIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);

    } else {
      insertNewValueIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);
    }

    //    insertNewValueIntoNode(insertStack, significantKeyInformation, insertDepth, leafDepth, valueToInsert);
#ifdef PING_DEBUG3
    std::cout << "New.......insertWithInsertStack function () end------ " << std::endl;
#endif
  });

}



/*
 * Insert the prefix rule, which is the rules has the wildcard at the end of the bit positions
 * insert the rule to the inner BiNode, related to the DB bit
 *
 */
//inline bool insertPrefixRule(unsigned int leafDepth, ValueType & value, uint64_t mask) {

//}

template<typename ValueType, template <typename> typename KeyExtractor> inline idx::contenthelpers::OptionalValue<ValueType> HOTSingleThreaded<ValueType, KeyExtractor>::upsert(ValueType newValue) {
  KeyType newKey = extractKey(newValue);
  auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(newValue)));
  uint8_t const* keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

  if(isRootANode()) {
    std::array<HOTSingleThreadedInsertStackEntry, 64> insertStack;
    unsigned int leafDepth = searchForInsert(keyBytes, insertStack);
    intptr_t tid = insertStack[leafDepth].mChildPointer->getTid();
    ValueType const & existingValue = idx::contenthelpers::tidToValue<ValueType>(tid);

    if(insertWithInsertStack(insertStack, leafDepth, extractKey(existingValue), keyBytes, newValue)) {
      return idx::contenthelpers::OptionalValue<ValueType>();
    } else {
      *insertStack[leafDepth].mChildPointer = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(newValue));
      return idx::contenthelpers::OptionalValue<ValueType>(true, existingValue);;
    }
  } else if(mRoot->isLeaf()) {
    ValueType existingValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());
    if(idx::contenthelpers::contentEquals(extractKey(existingValue), newKey)) {
      *mRoot = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(newValue));
      return { true, existingValue };
    } else {
      insert(newValue);
      return {};
    }
  } else {
    *mRoot = HOTSingleThreadedChildPointer(idx::contenthelpers::valueToTid(newValue));
    return {};
  }
}

template<typename ValueType, template <typename> typename KeyExtractor> inline typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::begin() const {
  return isEmpty() ? END_ITERATOR : const_iterator(mRoot);
}

template<typename ValueType, template <typename> typename KeyExtractor> inline typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::end() const {
  return END_ITERATOR;
}

template<typename ValueType, template <typename> typename KeyExtractor> inline typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::find(typename HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const & searchKey) const {
  return isRootANode() ? findForNonEmptyTrie(searchKey) : END_ITERATOR;
}

template<typename ValueType, template <typename> typename KeyExtractor> inline typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::findForNonEmptyTrie(typename HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const & searchKey) const {
  HOTSingleThreadedChildPointer const * current = mRoot;

  auto const & fixedSizedSearchKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(searchKey)));
  uint8_t const* searchKeyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizedSearchKey);

  HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator it(current, current + 1);
  while(!current->isLeaf()) {
    current = it.descend(current->executeForSpecificNodeType(true, [&](auto & node) {
      return node.search(searchKeyBytes);
    }), current->getNode()->end());
  }

  ValueType const & leafValue = idx::contenthelpers::tidToValue<ValueType>(current->getTid());

  return idx::contenthelpers::contentEquals(extractKey(leafValue), searchKey) ? it : END_ITERATOR;
}

template<typename ValueType, template <typename> typename KeyExtractor> inline  __attribute__((always_inline)) typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::lower_bound(typename HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const & searchKey) const {
  return lower_or_upper_bound(searchKey, true);
}

template<typename ValueType, template <typename> typename KeyExtractor> inline  __attribute__((always_inline)) typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::upper_bound(typename HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const & searchKey) const {
  return lower_or_upper_bound(searchKey, false);
}

template<typename ValueType, template <typename> typename KeyExtractor> inline  __attribute__((always_inline)) typename HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator HOTSingleThreaded<ValueType, KeyExtractor>::lower_or_upper_bound(typename HOTSingleThreaded<ValueType, KeyExtractor>::KeyType const & searchKey, bool is_lower_bound) const {
  if(isEmpty()) {
    return END_ITERATOR;
  }

  HOTSingleThreaded<ValueType, KeyExtractor>::const_iterator it(mRoot, mRoot + 1);

  if(mRoot->isLeaf()) {
    ValueType const & existingValue = idx::contenthelpers::tidToValue<ValueType>(mRoot->getTid());
    KeyType const & existingKey = extractKey(existingValue);

    return (idx::contenthelpers::contentEquals(searchKey, existingKey) || compareKeys(existingKey, searchKey)) ? it : END_ITERATOR;
  } else {
    auto const & fixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(searchKey));
    uint8_t const* keyBytes = idx::contenthelpers::interpretAsByteArray(fixedSizeKey);

    HOTSingleThreadedChildPointer const * current = mRoot;
    std::array<uint16_t, 64> mostSignificantBitIndexes;

    while(!current->isLeaf()) {
      current = it.descend(current->executeForSpecificNodeType(true, [&](auto & node) {
        mostSignificantBitIndexes[it.mCurrentDepth] = node.mDiscriminativeBitsRepresentation.mMostSignificantDiscriminativeBitIndex;
        return node.search(keyBytes);
      }), current->getNode()->end());
    }

    ValueType const & existingValue = *it;
    auto const & existingFixedSizeKey = idx::contenthelpers::toFixSizedKey(idx::contenthelpers::toBigEndianByteOrder(extractKey(existingValue)));
    uint8_t const* existingKeyBytes = idx::contenthelpers::interpretAsByteArray(existingFixedSizeKey);

    bool keysDiff = hot::commons::executeForDiffingKeys(existingKeyBytes, keyBytes, idx::contenthelpers::getMaxKeyLength<KeyType>(), [&](hot::commons::DiscriminativeBit const & significantKeyInformation) {
        //searches for the node to insert the new value into.
        //Be aware that this can result in a false positive. Therefor in case only a single entry is affected and it has a child node it must be inserted into the child node
        //this is an alternative approach to using getLeastSignificantDiscriminativeBitForEntry
        HOTSingleThreadedChildPointer const * child = it.mNodeStack[it.mCurrentDepth].getCurrent();
        unsigned int entryIndex = child - it.mNodeStack[--it.mCurrentDepth].getCurrent()->getNode()->getPointers();
        while (it.mCurrentDepth > 0 && significantKeyInformation.mAbsoluteBitIndex < mostSignificantBitIndexes[it.mCurrentDepth]) {
      child = it.mNodeStack[it.mCurrentDepth].getCurrent();
      entryIndex = child - it.mNodeStack[--it.mCurrentDepth].getCurrent()->getNode()->getPointers();
    }

    HOTSingleThreadedChildPointer const* currentNode = it.mNodeStack[it.mCurrentDepth].getCurrent();
    currentNode->executeForSpecificNodeType(false, [&](auto const &existingNode) -> void {
      hot::commons::InsertInformation const & insertInformation = existingNode.getInsertInformation(entryIndex, significantKeyInformation);

      unsigned int nextEntryIndex = insertInformation.mKeyInformation.mValue
          ? (insertInformation.getFirstIndexInAffectedSubtree() + insertInformation.getNumberEntriesInAffectedSubtree())
          : insertInformation.getFirstIndexInAffectedSubtree();

      HOTSingleThreadedChildPointer const * nextEntry = existingNode.getPointers() + nextEntryIndex;
      HOTSingleThreadedChildPointer const * endPointer = existingNode.end();

      it.descend(nextEntry, endPointer);

      if(nextEntry == endPointer) {
        ++it;
      } else {
        it.descend();
      }
    });
  });

  if(!keysDiff && !is_lower_bound) {
    ++it;
  }

  return it;
}
}



inline void insertNewWildcardRuleIntoNode(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, hot::commons::DiscriminativeBit const & significantKeyInformation, unsigned int insertDepth, unsigned int leafDepth, HOTSingleThreadedChildPointer const & valueToInsert) {
#ifdef PING_DEBUG9
  std::cout << "-----insertNewWildcardRuleIntoNode function begin: " << std::endl;
  std::cout << "------valueToInsert.mPointer = " << std::bitset<64>(valueToInsert.mPointer) << std::endl;
  printf("--------------valueToInsert.mMask = %lu\n", valueToInsert.mMask);
  std::cout << "insertDepth = " << insertDepth << ", " << "leafDepth = "  << leafDepth << std::endl;
#endif

  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];

  insertStackEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto &existingNode) -> void {
    uint32_t entryIndex = insertStackEntry.mSearchResultForInsert.mEntryIndex;
#ifdef PING_DEBUG9
    printf("-----entryIndex = %d---------\n", entryIndex);
#endif
    hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
          entryIndex, significantKeyInformation
          );


    #ifdef PING_DEBUG9
    size_t number_entries = existingNode.getNumberEntries();
    std::cout << "------Node.number_entries = " << number_entries << endl;
#endif
    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask
    bool isSingleEntry = (insertInformation.getNumberEntriesInAffectedSubtree() == 1);
    unsigned int nextInsertDepth = insertDepth + 1u;

    bool isLeafEntry = (nextInsertDepth == leafDepth);
#ifdef PING_DEBUG9
    printf("-------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif

    hot::commons::entryVector tmp;
    tmp.value = valueToInsert.mPointer;
    tmp.mask = valueToInsert.mMask;
    tmp.pp = valueToInsert.priority;


    /*
     * find the related entryindex: first index and number of entries
     */

    uint32_t firstIndex = insertInformation.mFirstIndexInAffectedSubtree;
    uint32_t numberEntries = insertInformation.mNumberEntriesInAffectedSubtree;

#ifdef PING_DEBUG9
    printf("-----Affected first entry index = %d-----, affected numberEntries = %d\n", firstIndex, numberEntries);
#endif

    if(isSingleEntry & isLeafEntry) {
#ifdef PING_DEBUG9
      printf("-------Only one entry-------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif

      //in case the current partition is a leaf partition add it to the partition
      //otherwise create new leaf partition containing the existing leaf node and the new value
      uint8_t & test2 = (existingNode.getPointers() + firstIndex)->listSize;

#ifdef PING_DEBUG9
      printf("--------listSize = %i-----\n", (existingNode.getPointers() + firstIndex)->listSize);
#endif

      (existingNode.getPointers() + firstIndex)->entryList[test2] = tmp;
      wildRule_count++;
      test2++;

#ifdef PING_DEBUG9
      printf("-----------after push_back--------listSize = %i-----\n", (*(existingNode.getPointers() + firstIndex)).listSize);
#endif


    } else if(isSingleEntry) {
#ifdef PING_DEBUG9
      printf("-------just singleEntry, but not leafEntry--------\n");
      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif



    } else {
#ifdef PING_DEBUG9
      printf("-----------not singleEntry------\n");
      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif


      for (uint i = 0; i < numberEntries; i++) {
#ifdef PING_DEBUG9
        printf("---------ping ping ping-----------------\n");
        std::cout << "-------.mPointer = " << std::bitset<64>((*(existingNode.getPointers() + firstIndex + i)).mPointer) << ", .mMask = " << (*(existingNode.getPointers() + firstIndex + i)).mMask << std::endl;
        std::cout << "-------value     = " <<  std::bitset<64>((*(existingNode.getPointers() + firstIndex + i)).mPointer >> (uint64_t)(1)) << std::endl;
        std::cout << "-------.priority = " << (*(existingNode.getPointers() + firstIndex + i)).priority << std::endl;
#endif

        uint8_t & test1 = (existingNode.getPointers() + firstIndex + i)->listSize;

#ifdef PING_DEBUG9
        printf("-------i = %d------listSize = %i-----\n", i, (existingNode.getPointers() + firstIndex + i)->listSize);
#endif

        (existingNode.getPointers() + firstIndex + i)->entryList[test1] = tmp;
        wildRule_count++;
        test1++;

#ifdef PING_DEBUG9
        printf("-----------after push_back-----i = %u--listSize = %i-----\n", i, (*(existingNode.getPointers() + firstIndex + i)).listSize);
#endif

      }



    }


    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask

  });

#ifdef PING_DEBUG9
  printf("----------insertNewWildcardRuleIntoNode function------ends------------------\n");
#endif

}

/*
 * added argument wildcard_affected_entry_mask
 * to make the affectedSubtreeEntry correct and reduce the number of inserted wildcard rules
 *
 */

//inline void insertNewWildcardRuleIntoNode(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, hot::commons::DiscriminativeBit const & significantKeyInformation, unsigned int insertDepth, unsigned int leafDepth, HOTSingleThreadedChildPointer const & valueToInsert) {
//#ifdef PING_DEBUG9
//  std::cout << "-----insertNewWildcardRuleIntoNode function begin: " << std::endl;
//  std::cout << "------valueToInsert.mPointer = " << std::bitset<64>(valueToInsert.mPointer) << std::endl;
//  printf("--------------valueToInsert.mMask = %lu\n", valueToInsert.mMask);
//  std::cout << "insertDepth = " << insertDepth << ", " << "leafDepth = "  << leafDepth << std::endl;
//#endif

//  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];

//  insertStackEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto &existingNode) -> void {
//    uint32_t entryIndex = insertStackEntry.mSearchResultForInsert.mEntryIndex;
//#ifdef PING_DEBUG9
//    printf("-----entryIndex = %d---------\n", entryIndex);
//#endif
////    hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
////          entryIndex, significantKeyInformation
////          );
//    assert(wildcard_affected_entry_mask != 0);
//    /*
//     * find the related entryindex: first index and number of entries
//     */
//    uint32_t firstIndex = __builtin_ctz(wildcard_affected_entry_mask);
//    uint32_t numberEntries = _mm_popcnt_u32(wildcard_affected_entry_mask);

//    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask
//    bool isSingleEntry = (numberEntries == 1);
//    unsigned int nextInsertDepth = insertDepth + 1u;

//    bool isLeafEntry = (nextInsertDepth == leafDepth);
//#ifdef PING_DEBUG9
//    printf("-------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
//#endif

//    hot::commons::entryVector tmp;
//    tmp.value = valueToInsert.mPointer;
//    tmp.mask = valueToInsert.mMask;
//    tmp.pp = valueToInsert.priority;




//#ifdef PING_DEBUG9
//    printf("-----Affected first entry index = %d-----, affected numberEntries = %d\n", firstIndex, numberEntries);
//#endif

//    if(isSingleEntry & isLeafEntry) {
//#ifdef PING_DEBUG9
//      printf("-------Only one entry-------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
//#endif

//      //in case the current partition is a leaf partition add it to the partition
//      //otherwise create new leaf partition containing the existing leaf node and the new value
//      uint8_t & test2 = (existingNode.getPointers() + firstIndex)->listSize;

//#ifdef PING_DEBUG9
//      printf("--------listSize = %i-----\n", (existingNode.getPointers() + firstIndex)->listSize);
//#endif

//      (existingNode.getPointers() + firstIndex)->entryList[test2] = tmp;
//      wildRule_count++;
//      test2++;

//#ifdef PING_DEBUG9
//      printf("-----------after push_back--------listSize = %i-----\n", (*(existingNode.getPointers() + firstIndex)).listSize);
//#endif


//    } else if(isSingleEntry) {
//#ifdef PING_DEBUG9
//      printf("-------just singleEntry, but not leafEntry--------\n");
//      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
//#endif



//    } else {
//#ifdef PING_DEBUG9
//      printf("-----------not singleEntry------\n");
//      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
//#endif


//      for (uint i = 0; i < numberEntries; i++) {
//#ifdef PING_DEBUG9
//        printf("---------ping ping ping-----------------\n");
//        std::cout << "-------.mPointer = " << std::bitset<64>((*(existingNode.getPointers() + firstIndex + i)).mPointer) << ", .mMask = " << (*(existingNode.getPointers() + firstIndex + i)).mMask << std::endl;
//        std::cout << "-------value     = " <<  std::bitset<64>((*(existingNode.getPointers() + firstIndex + i)).mPointer >> (uint64_t)(1)) << std::endl;
//        std::cout << "-------.priority = " << (*(existingNode.getPointers() + firstIndex + i)).priority << std::endl;
//#endif

//        uint8_t & test1 = (existingNode.getPointers() + firstIndex + i)->listSize;

//#ifdef PING_DEBUG9
//        printf("-------i = %d------listSize = %i-----\n", i, (existingNode.getPointers() + firstIndex + i)->listSize);
//#endif

//        (existingNode.getPointers() + firstIndex + i)->entryList[test1] = tmp;
//        wildRule_count++;
//        test1++;

//#ifdef PING_DEBUG9
//        printf("-----------after push_back-----i = %u--listSize = %i-----\n", i, (*(existingNode.getPointers() + firstIndex + i)).listSize);
//#endif

//      }



//    }


//    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask

//  });

//#ifdef PING_DEBUG9
//  printf("----------insertNewWildcardRuleIntoNode function------ends------------------\n");
//#endif

//}


inline void insertNewValueIntoNode(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, hot::commons::DiscriminativeBit const & significantKeyInformation, unsigned int insertDepth, unsigned int leafDepth, HOTSingleThreadedChildPointer const & valueToInsert) {
#ifdef PING_DEBUG3
  std::cout << "-----------------insertNewValueIntoNode function begin: " << std::endl;
  std::cout << "valueToInsert.mPointer = " << std::bitset<64>(valueToInsert.mMask) << std::endl;
  printf("------------valueToInsert.mMask = %lu\n", valueToInsert.mMask);
  std::cout << "insertDepth = " << insertDepth << ", " << "leafDepth = "  << leafDepth << std::endl;
#endif
  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];

  insertStackEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto const &existingNode) -> void {
    uint32_t entryIndex = insertStackEntry.mSearchResultForInsert.mEntryIndex;
#ifdef PING_DEBUG3
    printf("entryIndex = %d\n", entryIndex);
#endif
    hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
          entryIndex, significantKeyInformation
          );

    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask
    bool isSingleEntry = (insertInformation.getNumberEntriesInAffectedSubtree() == 1);
    unsigned int nextInsertDepth = insertDepth + 1u;

    bool isLeafEntry = (nextInsertDepth == leafDepth);
#ifdef PING_DEBUG3
    printf("isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif
    if(isSingleEntry & isLeafEntry) {
#ifdef PING_DEBUG3
      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif
      HOTSingleThreadedChildPointer const & leafEntry = *insertStack[nextInsertDepth].mChildPointer;
      //in case the current partition is a leaf partition add it to the partition
      //otherwise create new leaf partition containing the existing leaf node and the new value
      integrateBiNodeIntoTree(insertStack, nextInsertDepth, hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                                insertInformation.mKeyInformation, leafEntry, valueToInsert
                                ), true);
    } else if(isSingleEntry) { //in this case the single entry is a boundary node -> insert the value into the child partition
#ifdef PING_DEBUG3
      printf("-------just singleEntry, but not leafEntry--------\n");
      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif
      insertStack[nextInsertDepth].mChildPointer->executeForSpecificNodeType(false, [&](auto &childPartition) -> void {
        insertNewValueResultingInNewPartitionRoot(childPartition, insertStack, significantKeyInformation, nextInsertDepth,
                                                  valueToInsert);
      });
    } else {
#ifdef PING_DEBUG3
      printf("-----------not singleEntry------\n");
      printf("----------isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif
      insertNewValue(existingNode, insertStack, insertInformation, insertDepth, valueToInsert);
    }
  });
#ifdef PING_DEBUG3
  std::cout << "---------------------------------------------insertNewValueIntoNode function-----finish---" << std::endl;
#endif
}



inline void insertNewValueIntoNode(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, hot::commons::DiscriminativeBit const & significantKeyInformation, unsigned int insertDepth, unsigned int leafDepth, HOTSingleThreadedChildPointer const & valueToInsert, uint64_t mask) {
#ifdef PING_DEBUG3
  std::cout << "----New insertNewValueIntoNode function ----with mask argument------begin: " << std::endl;
  uint8_t *val = (uint8_t*)(&valueToInsert);
  printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
  //printf("valueToInsert = %p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
  std::cout << "insertDepth = " << insertDepth << ", " << "leafDepth = "  << leafDepth << std::endl;
#endif
  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];

  insertStackEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto const &existingNode) -> void {
    uint32_t entryIndex = insertStackEntry.mSearchResultForInsert.mEntryIndex;
#ifdef PING_DEBUG3
    printf("entryIndex = %d\n", entryIndex);
#endif
    hot::commons::InsertInformation const &insertInformation = existingNode.getInsertInformation(
          entryIndex, significantKeyInformation
          );

    //As entryMask hasOnly a single bit set insertInformation.mAffectedSubtreeMask == entryMask checks whether the entry bit is the only bit set in the affectedSubtreeMask
    bool isSingleEntry = (insertInformation.getNumberEntriesInAffectedSubtree() == 1);
    unsigned int nextInsertDepth = insertDepth + 1u;

    bool isLeafEntry = (nextInsertDepth == leafDepth);
#ifdef PING_DEBUG3
    printf("isSingleEntry = %d, isLeafEntry = %d\n", isSingleEntry, isLeafEntry);
#endif
    if(isSingleEntry & isLeafEntry) {
      HOTSingleThreadedChildPointer const & leafEntry = *insertStack[nextInsertDepth].mChildPointer;
      //in case the current partition is a leaf partition add it to the partition
      //otherwise create new leaf partition containing the existing leaf node and the new value
      integrateBiNodeIntoTree(insertStack, nextInsertDepth, hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(
                                insertInformation.mKeyInformation, leafEntry, valueToInsert
                                ), true);
    } else if(isSingleEntry) { //in this case the single entry is a boundary node -> insert the value into the child partition
      insertStack[nextInsertDepth].mChildPointer->executeForSpecificNodeType(false, [&](auto &childPartition) -> void {
        insertNewValueResultingInNewPartitionRoot(childPartition, insertStack, significantKeyInformation, nextInsertDepth,
                                                  valueToInsert);
      });
    } else {
      insertNewValue(existingNode, insertStack, insertInformation, insertDepth, valueToInsert);
    }
  });
  //  insertStackEntry.mChildPointer->mMask = mask;

#ifdef PING_DEBUG3
  std::cout << "the mask get updated---" << std::endl;
#endif

#ifdef PING_DEBUG3
  std::cout << "insertNewValueIntoNode function-----finish---" << std::endl;
#endif
}





template<typename NodeType> inline void insertNewValueResultingInNewPartitionRoot(NodeType const &existingNode,
                                                                                  std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                                                                                  const hot::commons::DiscriminativeBit &keyInformation,
                                                                                  unsigned int insertDepth,
                                                                                  HOTSingleThreadedChildPointer const &valueToInsert) {
#ifdef PING_DEBUG
  printf("insertNewValueResultingInNewPartitionRoot function() begin\n");
#endif
  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];
  if (!existingNode.isFull()) {
    //As the insert results in a new partition root, no prefix bits are set and all entries in the partition are affected
#ifdef PING_DEBUG
    std::cout << "no prefix bits are set and all entries in the partition are affected" << std::endl;
    std::cout << "existingNode.getNumberEntries = " << existingNode.getNumberEntries() << std::endl;
    std::cout << "keyInformation: " << keyInformation.mByteIndex << ", " << keyInformation.mByteRelativeBitIndex << ", " <<
                 keyInformation.mAbsoluteBitIndex << ", " << keyInformation.mValue << std::endl;
#endif
    hot::commons::InsertInformation insertInformation { 0, 0, static_cast<uint32_t>(existingNode.getNumberEntries()), keyInformation};
    *(insertStackEntry.mChildPointer) = existingNode.addEntry(insertInformation, valueToInsert);
    delete &existingNode;
  } else {
    assert(keyInformation.mAbsoluteBitIndex != insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
    hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(keyInformation, *insertStackEntry.mChildPointer, valueToInsert);
    integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
  }
#ifdef PING_DEBUG
  printf("insertNewValueResultingInNewPartitionRoot function()-------finish\n");
#endif
}

template<typename NodeType> inline void insertNewValue(NodeType const &existingNode,
                                                       std::array<HOTSingleThreadedInsertStackEntry, 64> &insertStack,
                                                       hot::commons::InsertInformation const &insertInformation,
                                                       unsigned int insertDepth, HOTSingleThreadedChildPointer const &valueToInsert) {
#ifdef PING_DEBUG
  printf("insertNewValue function() begin-----\n");
#endif
  //  uint64_t mask = valueToInsert.mMask;
  HOTSingleThreadedInsertStackEntry const & insertStackEntry = insertStack[insertDepth];

  if (!existingNode.isFull()) {
#ifdef PING_DEBUG
    printf("existing node is not full----\n");
#endif
    HOTSingleThreadedChildPointer newNodePointer = existingNode.addEntry(insertInformation, valueToInsert);
    *(insertStackEntry.mChildPointer) = newNodePointer;
    delete &existingNode;
  } else {
    assert(insertInformation.mKeyInformation.mAbsoluteBitIndex != insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex);
    if (insertInformation.mKeyInformation.mAbsoluteBitIndex > insertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex) {
      hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = existingNode.split(insertInformation, valueToInsert);
      integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
      delete &existingNode;
    } else {
      hot::commons::BiNode<HOTSingleThreadedChildPointer> const &binaryNode = hot::commons::BiNode<HOTSingleThreadedChildPointer>::createFromExistingAndNewEntry(insertInformation.mKeyInformation, *insertStackEntry.mChildPointer, valueToInsert);
      integrateBiNodeIntoTree(insertStack, insertDepth, binaryNode, true);
    }
  }
#ifdef PING_DEBUG
  printf("insertNewValue function()-------finish\n");
#endif
}



inline void integrateBiNodeIntoTree(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, unsigned int currentDepth, hot::commons::BiNode<HOTSingleThreadedChildPointer> const & splitEntries, bool const newIsRight) {
#ifdef PING_DEBUG3
  printf("integrateBiNodeIntoTree function() begin--------:\n");
  printf("currentDepth = %d, newIsRight = %d\n", currentDepth, newIsRight);
#endif
  if(currentDepth == 0) {
#ifdef PING_DEBUG3
    printf("mRoot is the leaf node\n");
#endif
    *insertStack[0].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(splitEntries)->toChildPointer();
  } else {
    unsigned int parentDepth = currentDepth - 1;
#ifdef PING_DEBUG3
    printf("parentDepth = %d\n", parentDepth);
#endif
    HOTSingleThreadedInsertStackEntry const & parentInsertStackEntry = insertStack[parentDepth];
    HOTSingleThreadedChildPointer parentNodePointer = *parentInsertStackEntry.mChildPointer;
#ifdef PING_DEBUG3
    uint8_t *val = (uint8_t*)&parentNodePointer.mPointer;
    std::cout << "parentNodePointer.value =  " << std::bitset<64>(*val) << std::endl;
#endif
    HOTSingleThreadedNodeBase* existingParentNode = parentNodePointer.getNode();
#ifdef PING_DEBUG3
    uint8_t *val1 = (uint8_t*)&existingParentNode;
    std::cout << "existingParentNode.value = " << std::bitset<64>(*val1) << std::endl;;
    printf("existingParentNode->mHeight = %d, splitEntries.mHeight = %d\n", existingParentNode->mHeight, splitEntries.mHeight);
#endif
    if(existingParentNode->mHeight > splitEntries.mHeight) { //create intermediate partition if height(partition) + 1 < height(parentPartition)
      *insertStack[currentDepth].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(splitEntries)->toChildPointer();
    } else { //integrate nodes into parent partition
#ifdef PING_DEBUG3
      printf("splitEntries.mDiscriminativeBitIndex = %d\n", splitEntries.mDiscriminativeBitIndex);
#endif
      hot::commons::DiscriminativeBit const significantKeyInformation { splitEntries.mDiscriminativeBitIndex, newIsRight };

      parentNodePointer.executeForSpecificNodeType(false, [&](auto & parentNode) -> void {
        hot::commons::InsertInformation const & insertInformation = parentNode.getInsertInformation(
              parentInsertStackEntry.mSearchResultForInsert.mEntryIndex, significantKeyInformation
              );

        unsigned int entryOffset = (newIsRight) ? 0 : 1;
        HOTSingleThreadedChildPointer valueToInsert { (newIsRight) ? splitEntries.mRight : splitEntries.mLeft};
                                                      HOTSingleThreadedChildPointer valueToReplace { (newIsRight) ? splitEntries.mLeft : splitEntries.mRight };

                                                                                                     if(!parentNode.isFull()) {
                                                                                                       HOTSingleThreadedChildPointer newNodePointer = parentNode.addEntry(insertInformation, valueToInsert);
                                                                                                       newNodePointer.getNode()->getPointers()[parentInsertStackEntry.mSearchResultForInsert.mEntryIndex + entryOffset] = valueToReplace;
                                                                                                       *parentInsertStackEntry.mChildPointer = newNodePointer;
                                                                                                     } else {
                                                                                                       //The diffing Bit index cannot be larger as the parents mostSignificantBitIndex. the reason is that otherwise
                                                                                                       //the trie condition would be violated
                                                                                                       assert(parentInsertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex < splitEntries.mDiscriminativeBitIndex);
                                                                                                       //Furthermore due to the trie condition it is safe to assume that both the existing entry and the new entry will be part of the same subtree
                                                                                                       hot::commons::BiNode<HOTSingleThreadedChildPointer> const & newSplitEntries = parentNode.split(insertInformation, valueToInsert);
                                                                                                       //Detect subtree side
                                                                                                       //This newSplitEntries.mLeft.getHeight() == parentNodePointer.getHeight() check is important because in case of a split with 1:31 it can happend that if
                                                                                                       //the 1 entry is not a leaf node the node it is pointing to will be pulled up, which implies that the numberEntriesInLowerPart are not correct anymore.
                                                                                                       unsigned int numberEntriesInLowerPart = newSplitEntries.mLeft.getHeight() == parentNode.mHeight ? newSplitEntries.mLeft.getNumberEntries() : 1;
                                                                                                       bool isInUpperPart = numberEntriesInLowerPart <= parentInsertStackEntry.mSearchResultForInsert.mEntryIndex;
                                                                                                       //Here is problem because of parentInsertstackEntry
                                                                                                       unsigned int correspondingEntryIndexInPart = parentInsertStackEntry.mSearchResultForInsert.mEntryIndex - (isInUpperPart * numberEntriesInLowerPart) + entryOffset;
                                                                                                       HOTSingleThreadedChildPointer nodePointerContainingSplitEntries = (isInUpperPart) ? newSplitEntries.mRight : newSplitEntries.mLeft;
                                                                                                       nodePointerContainingSplitEntries.getNode()->getPointers()[correspondingEntryIndexInPart] = valueToReplace;
                                                                                                       integrateBiNodeIntoTree(insertStack, parentDepth, newSplitEntries, true);
                                                                                                     }
                                                                                                                                                  delete &parentNode;
                                                                                                   });




                                                                                                  //Order because branch prediction might choose this case in first place

                                                    }
      }
#ifdef PING_DEBUG3
      printf("integrateBiNodeIntoTree function() ------finish\n");

    #endif


    }


    inline void integrateBiNodeIntoTree(std::array<HOTSingleThreadedInsertStackEntry, 64> & insertStack, unsigned int currentDepth, hot::commons::BiNode<HOTSingleThreadedChildPointer> const & splitEntries, bool const newIsRight, uint64_t mask) {
#ifdef PING_DEBUG3
      printf("====New integrateBiNodeIntoTree function() begin----added Mask----:\n");
      printf("currentDepth = %d, newIsRight = %d\n", currentDepth, newIsRight);
#endif
      if(currentDepth == 0) {
#ifdef PING_DEBUG3
        printf("mRoot is the leaf node\n");
#endif
        *insertStack[0].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(splitEntries)->toChildPointer();
      } else {
        unsigned int parentDepth = currentDepth - 1;
#ifdef PING_DEBUG3
        printf("parentDepth = %d\n", parentDepth);
#endif
        HOTSingleThreadedInsertStackEntry const & parentInsertStackEntry = insertStack[parentDepth];
        HOTSingleThreadedChildPointer parentNodePointer = *parentInsertStackEntry.mChildPointer;
        HOTSingleThreadedNodeBase* existingParentNode = parentNodePointer.getNode();

        if(existingParentNode->mHeight > splitEntries.mHeight) { //create intermediate partition if height(partition) + 1 < height(parentPartition)
          *insertStack[currentDepth].mChildPointer = hot::commons::createTwoEntriesNode<HOTSingleThreadedChildPointer, HOTSingleThreadedNode>(splitEntries)->toChildPointer();
        } else { //integrate nodes into parent partition
          hot::commons::DiscriminativeBit const significantKeyInformation { splitEntries.mDiscriminativeBitIndex, newIsRight };

          parentNodePointer.executeForSpecificNodeType(false, [&](auto & parentNode) -> void {
            hot::commons::InsertInformation const & insertInformation = parentNode.getInsertInformation(
                  parentInsertStackEntry.mSearchResultForInsert.mEntryIndex, significantKeyInformation
                  );

            unsigned int entryOffset = (newIsRight) ? 0 : 1;
            HOTSingleThreadedChildPointer valueToInsert { (newIsRight) ? splitEntries.mRight : splitEntries.mLeft};
                                                          HOTSingleThreadedChildPointer valueToReplace { (newIsRight) ? splitEntries.mLeft : splitEntries.mRight};
                                                                                                         printf("===================valuetoInsert.mMask = %lu, valueToReplace.mMask = %lu\n", valueToInsert.mMask, valueToReplace.mMask);
                                                                                                                                                      valueToInsert.mMask = mask;
                                                                                                                                                                                                   printf("=======again============valuetoInsert.mMask = %lu, valueToReplace.mMask = %lu\n", valueToInsert.mMask, valueToReplace.mMask);


                                                                                                                                                                                                                                                //                                                         HOTSingleThreadedChildPointer valueToInsert ( (newIsRight) ? splitEntries.mRight : splitEntries.mLeft, mask );
                                                                                                                                                                                                                                                //                                                                                                      HOTSingleThreadedChildPointer valueToReplace ( (newIsRight) ? splitEntries.mLeft : splitEntries.mRight, mask );

                                                                                                                                                                                                                                                if(!parentNode.isFull()) {
                                                                                                                                                                                                                                                  HOTSingleThreadedChildPointer newNodePointer = parentNode.addEntry(insertInformation, valueToInsert);
                                                                                                                                                                                                                                                  newNodePointer.getNode()->getPointers()[parentInsertStackEntry.mSearchResultForInsert.mEntryIndex + entryOffset] = valueToReplace;
                                                                                                                                                                                                                                                  *parentInsertStackEntry.mChildPointer = newNodePointer;
                                                                                                                                                                                                                                                } else {
                                                                                                                                                                                                                                                  //The diffing Bit index cannot be larger as the parents mostSignificantBitIndex. the reason is that otherwise
                                                                                                                                                                                                                                                  //the trie condition would be violated
                                                                                                                                                                                                                                                  assert(parentInsertStackEntry.mSearchResultForInsert.mMostSignificantBitIndex < splitEntries.mDiscriminativeBitIndex);
                                                                                                                                                                                                                                                  //Furthermore due to the trie condition it is safe to assume that both the existing entry and the new entry will be part of the same subtree
                                                                                                                                                                                                                                                  hot::commons::BiNode<HOTSingleThreadedChildPointer> const & newSplitEntries = parentNode.split(insertInformation, valueToInsert);
                                                                                                                                                                                                                                                  //Detect subtree side
                                                                                                                                                                                                                                                  //This newSplitEntries.mLeft.getHeight() == parentNodePointer.getHeight() check is important because in case of a split with 1:31 it can happend that if
                                                                                                                                                                                                                                                  //the 1 entry is not a leaf node the node it is pointing to will be pulled up, which implies that the numberEntriesInLowerPart are not correct anymore.
                                                                                                                                                                                                                                                  unsigned int numberEntriesInLowerPart = newSplitEntries.mLeft.getHeight() == parentNode.mHeight ? newSplitEntries.mLeft.getNumberEntries() : 1;
                                                                                                                                                                                                                                                  bool isInUpperPart = numberEntriesInLowerPart <= parentInsertStackEntry.mSearchResultForInsert.mEntryIndex;
                                                                                                                                                                                                                                                  //Here is problem because of parentInsertstackEntry
                                                                                                                                                                                                                                                  unsigned int correspondingEntryIndexInPart = parentInsertStackEntry.mSearchResultForInsert.mEntryIndex - (isInUpperPart * numberEntriesInLowerPart) + entryOffset;
                                                                                                                                                                                                                                                  HOTSingleThreadedChildPointer nodePointerContainingSplitEntries = (isInUpperPart) ? newSplitEntries.mRight : newSplitEntries.mLeft;
                                                                                                                                                                                                                                                  nodePointerContainingSplitEntries.getNode()->getPointers()[correspondingEntryIndexInPart] = valueToReplace;
                                                                                                                                                                                                                                                  integrateBiNodeIntoTree(insertStack, parentDepth, newSplitEntries, true);
                                                                                                                                                                                                                                                }
                                                                                                                                                                                                                                                                                             delete &parentNode;
                                                                                                       });




                                                                                                      //Order because branch prediction might choose this case in first place

                                                        }
          }
#ifdef PING_DEBUG3
          printf("integrateBiNodeIntoTree function() ------finish\n");

    #endif


        }






        template<typename ValueType, template <typename> typename KeyExtractor> inline HOTSingleThreadedChildPointer HOTSingleThreaded<ValueType, KeyExtractor>::getNodeAtPath(std::initializer_list<unsigned int> path) {
          HOTSingleThreadedChildPointer current = *mRoot;
          for(unsigned int entryIndex : path) {
            assert(!current.isLeaf());
            current = current.getNode()->getPointers()[entryIndex];
          }
          return current;
        }

        template<typename ValueType, template <typename> typename KeyExtractor> inline void HOTSingleThreaded<ValueType, KeyExtractor>::collectStatsForSubtree(HOTSingleThreadedChildPointer const & subTreeRoot, std::map<std::string, double> & stats) const {
          if(!subTreeRoot.isLeaf()) {
            subTreeRoot.executeForSpecificNodeType(true, [&, this](auto & node) -> void {
              std::string nodeType = nodeAlgorithmToString(node.mNodeType);
              stats["total"] += node.getNodeSizeInBytes();
              stats[nodeType] += 1.0;
              for(HOTSingleThreadedChildPointer const & childPointer : node) {
                this->collectStatsForSubtree(childPointer, stats);
              }
            });
          }
        }

        template<typename ValueType, template <typename> typename KeyExtractor> std::pair<size_t, std::map<std::string, double>> HOTSingleThreaded<ValueType, KeyExtractor>::getStatistics() const {
#ifdef PING_DEBUG14
          std::cout << "HOTSingleThreaded_getStatistics() function begins-----" << std::endl;
#endif
          std::map<size_t, size_t> leafNodesPerDepth;
          getValueDistribution(*mRoot, 0, leafNodesPerDepth);
          std::map<size_t, size_t> leafNodesPerBinaryDepth;
          getBinaryTrieValueDistribution(*mRoot, 0, leafNodesPerBinaryDepth);

          std::map<std::string, double> statistics;
          statistics["height"] = mRoot->getHeight();
          statistics["numberAllocations"] = HOTSingleThreadedNodeBase::getNumberAllocations();

          size_t overallLeafNodeCount = 0;
          for(auto leafNodesOnDepth : leafNodesPerDepth) {
            std::string statisticsKey { "leafNodesOnDepth_"};
            std::string levelString = std::to_string(leafNodesOnDepth.first);
            statisticsKey += std::string(2 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnDepth.second;
            overallLeafNodeCount += leafNodesOnDepth.second;
          }

          for(auto leafNodesOnBinaryDepth : leafNodesPerBinaryDepth) {
            std::string statisticsKey { "leafNodesOnBinaryDepth_"};
            std::string levelString = std::to_string(leafNodesOnBinaryDepth.first);
            statisticsKey += std::string(3 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnBinaryDepth.second;
          }

          statistics["numberValues"] = overallLeafNodeCount;
          collectStatsForSubtree(*mRoot, statistics);

          size_t totalSize = statistics["total"];
          statistics.erase("total");
#ifdef PING_DEBUG14
          std::cout << "-------------------------=====HOTSingleThreaded_getStatistics() function ends-----" << std::endl;
#endif
          return {totalSize, statistics };
        }

        template<typename ValueType, template <typename> typename KeyExtractor> inline void HOTSingleThreaded<ValueType, KeyExtractor>::getValueDistribution(HOTSingleThreadedChildPointer const & childPointer, size_t depth, std::map<size_t, size_t> & leafNodesPerDepth) const {
#ifdef PING_DEBUG
          std::cout << "HOTSingleThreaded_getValueDistribution() function begins---- " << "depth = " << depth << std::endl;
          uint8_t *val = (uint8_t*)&childPointer;
          std::cout << "childPointer = ";
          printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
          if(childPointer.isLeaf()) {
#ifdef PING_DEBUG
            printf("childPointer.isLeaf() = true\n");
#endif
            ++leafNodesPerDepth[depth];
          } else {
#ifdef PING_DEBUG
            printf("childPointer.isLeaf() = false\n");
#endif
            for(HOTSingleThreadedChildPointer const & pointer : (*childPointer.getNode())) {
              getValueDistribution(pointer, depth + 1, leafNodesPerDepth);
            }
          }
#ifdef PING_DEBUG
          printf("=========getValueDistribution function () ends------\n");
#endif
        }


        template<typename ValueType, template <typename> typename KeyExtractor> std::pair<size_t, std::map<std::string, double>> HOTSingleThreaded<ValueType, KeyExtractor>::getStatistics_left() const {
#ifdef PING_DEBUG14
          std::cout << "HOTSingleThreaded_getStatistics() function begins--left---" << std::endl;
#endif
          std::map<size_t, size_t> leafNodesPerDepth;
          getValueDistribution(mRoot_left, 0, leafNodesPerDepth);
          std::map<size_t, size_t> leafNodesPerBinaryDepth;
          getBinaryTrieValueDistribution(mRoot_left, 0, leafNodesPerBinaryDepth);

          std::map<std::string, double> statistics;
          statistics["height"] = mRoot_left.getHeight();
          statistics["numberAllocations"] = HOTSingleThreadedNodeBase::getNumberAllocations();

          size_t overallLeafNodeCount = 0;
          for(auto leafNodesOnDepth : leafNodesPerDepth) {
            std::string statisticsKey { "leafNodesOnDepth_"};
            std::string levelString = std::to_string(leafNodesOnDepth.first);
            statisticsKey += std::string(2 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnDepth.second;
            overallLeafNodeCount += leafNodesOnDepth.second;
          }

          for(auto leafNodesOnBinaryDepth : leafNodesPerBinaryDepth) {
            std::string statisticsKey { "leafNodesOnBinaryDepth_"};
            std::string levelString = std::to_string(leafNodesOnBinaryDepth.first);
            statisticsKey += std::string(3 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnBinaryDepth.second;
          }

          statistics["numberValues"] = overallLeafNodeCount;
          collectStatsForSubtree(mRoot_left, statistics);

          size_t totalSize = statistics["total"];

          statistics.erase("total");
          std::cout << "left_total: " << totalSize << std::endl;
#ifdef PING_DEBUG14
          std::cout << "left tree total size = " << totalSize << std::endl;
          std::cout << "-------left tree------------------=====HOTSingleThreaded_getStatistics() function ends-----" << std::endl;
#endif
          return {totalSize, statistics };
        }



        template<typename ValueType, template <typename> typename KeyExtractor> std::pair<size_t, std::map<std::string, double>> HOTSingleThreaded<ValueType, KeyExtractor>::getStatistics_right() const {
#ifdef PING_DEBUG5
          std::cout << "HOTSingleThreaded_getStatistics() function begins---right--" << std::endl;
          printf("mRoot_right = %p\n", mRoot_right);
#endif
          std::map<size_t, size_t> leafNodesPerDepth;
          getValueDistribution(mRoot_right, 0, leafNodesPerDepth);
          std::map<size_t, size_t> leafNodesPerBinaryDepth;
          getBinaryTrieValueDistribution(mRoot_right, 0, leafNodesPerBinaryDepth);

          std::map<std::string, double> statistics;
          statistics["height"] = mRoot_right.getHeight();
          statistics["numberAllocations"] = HOTSingleThreadedNodeBase::getNumberAllocations();

          size_t overallLeafNodeCount = 0;
          for(auto leafNodesOnDepth : leafNodesPerDepth) {
            std::string statisticsKey { "leafNodesOnDepth_"};
            std::string levelString = std::to_string(leafNodesOnDepth.first);
            statisticsKey += std::string(2 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnDepth.second;
            overallLeafNodeCount += leafNodesOnDepth.second;
          }

          for(auto leafNodesOnBinaryDepth : leafNodesPerBinaryDepth) {
            std::string statisticsKey { "leafNodesOnBinaryDepth_"};
            std::string levelString = std::to_string(leafNodesOnBinaryDepth.first);
            statisticsKey += std::string(3 - levelString.length(), '0') + levelString;
            statistics[statisticsKey] = leafNodesOnBinaryDepth.second;
          }

          statistics["numberValues"] = overallLeafNodeCount;
          collectStatsForSubtree(mRoot_right, statistics);

          size_t totalSize = statistics["total"];
          statistics.erase("total");
          std::cout << "right_total: " << totalSize << std::endl;
#ifdef PING_DEBUG14
          std::cout << "right tree total size = " << totalSize << std::endl;
          std::cout << "----------right---------------=====HOTSingleThreaded_getStatistics() function ends-----" << std::endl;
#endif
          return {totalSize, statistics };
        }




        template<typename ValueType, template <typename> typename KeyExtractor> inline void HOTSingleThreaded<ValueType, KeyExtractor>::getBinaryTrieValueDistribution(HOTSingleThreadedChildPointer const & childPointer, size_t binaryTrieDepth, std::map<size_t, size_t> & leafNodesPerDepth) const {
#ifdef PING_DEBUG
          std::cout << "HOTSingleThreaded_getBinaryTrieValueDistribution() function: " << "binaryTrieDepth = " << binaryTrieDepth << std::endl;
          uint8_t *val = (uint8_t*)&childPointer;
          std::cout << "childPointer = ";
          printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5],val[6], val[7]);
#endif
          if(childPointer.isLeaf()) {
#ifdef PING_DEBUG
            printf("childPointer.isLeaf() = true\n");
#endif
            ++leafNodesPerDepth[binaryTrieDepth];
          } else {
#ifdef PING_DEBUG
            printf("childPointer.isLeaf() = false\n");
#endif
            childPointer.executeForSpecificNodeType(true, [&, this](auto &node) {
              std::array<uint8_t, 32> binaryEntryDepthsInNode = node.getEntryDepths();
              size_t i=0;
              for(HOTSingleThreadedChildPointer const & pointer : node) {
                this->getBinaryTrieValueDistribution(pointer, binaryTrieDepth + binaryEntryDepthsInNode[i], leafNodesPerDepth);
                ++i;
              }
            });
          }
        }

        template<typename ValueType, template <typename> typename KeyExtractor>
        bool HOTSingleThreaded<ValueType, KeyExtractor>::hasTheSameKey(intptr_t tid, HOTSingleThreaded<ValueType, KeyExtractor>::KeyType  const & key) {
          KeyType const & storedKey = extractKey(idx::contenthelpers::tidToValue<ValueType>(tid));
          return idx::contenthelpers::contentEquals(storedKey, key);
        }

        template<typename ValueType, template <typename> typename KeyExtractor>
        HOTSingleThreadedDeletionInformation HOTSingleThreaded<ValueType, KeyExtractor>::determineDeletionInformation(
              const std::array<HOTSingleThreadedInsertStackEntry, 64> &searchStack, unsigned int currentDepth) {
#ifdef PING_DEBUG
          printf("determineDeletionInformation function ()-----begins---\n");
          printf("--currentDepth = %d---\n", currentDepth);
#endif
          HOTSingleThreadedInsertStackEntry const & currentEntry = searchStack[currentDepth];
          uint32_t indexOfEntryToRemove = currentEntry.mSearchResultForInsert.mEntryIndex;
#ifdef PING_DEBUG
          printf("----indexOfEntryToRemove = %d\n", indexOfEntryToRemove);
#endif
          return currentEntry.mChildPointer->executeForSpecificNodeType(false, [&](auto const & currentNode) {
            return currentNode.getDeletionInformation(indexOfEntryToRemove);
          });
#ifdef PING_DEBUG
          printf("=========determineDeletionInformation function ()-----ends---\n");
#endif
        }

        template<typename ValueType, template <typename> typename KeyExtractor>
        size_t HOTSingleThreaded<ValueType, KeyExtractor>::getHeight() const {
          return isEmpty() ? 0 : mRoot->getHeight();

        }


      } }

#endif
