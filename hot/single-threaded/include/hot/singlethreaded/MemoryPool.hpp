#ifndef __HOT__SINGLE_THREADED__MEMORY_POOL__
#define __HOT__SINGLE_THREADED__MEMORY_POOL__

#include <algorithm>
#include <array>
#include <cassert>

namespace hot { namespace singlethreaded {

class FreeListEntry;

class FreeListEntry {
private:
  FreeListEntry* mNext;
  size_t mListSize;

public:
  FreeListEntry() : mNext(nullptr), mListSize(0) {

  }

  FreeListEntry(FreeListEntry* next) : mNext(next), mListSize(next->mListSize + 1) {
  }

  FreeListEntry* getNext() {
    return mNext;
  }

  size_t getListSize() {
    return mListSize;
  }
};

template<typename ElementType, size_t NUMBER_LISTS, size_t EVICTION_BEGIN_SIZE = 200, size_t EVICTION_END_SIZE = EVICTION_BEGIN_SIZE/2> class MemoryPool;

template<typename ElementType, size_t NUMBER_LISTS, size_t EVICTION_BEGIN_SIZE, size_t EVICTION_END_SIZE> class MemoryPool {
  static constexpr size_t SIZE_BEFORE_EVICTION_BEGIN_SIZE = EVICTION_BEGIN_SIZE - 1u;
  static constexpr size_t ELEMENT_SIZE = sizeof(ElementType);
  static FreeListEntry TERMINATING_ENTRY;

  std::array<FreeListEntry*, NUMBER_LISTS> mFreeLists;
  size_t mNumberAllocations;
  size_t mNumberFrees;

public:
  MemoryPool() : mNumberAllocations(0ul), mNumberFrees(0ul) {
    std::fill(mFreeLists.begin(), mFreeLists.end(), &TERMINATING_ENTRY);
  }

  MemoryPool(MemoryPool<ElementType, NUMBER_LISTS> const & other) = delete;
  MemoryPool& operator=(MemoryPool<ElementType, NUMBER_LISTS> const & other) = delete;

  ~MemoryPool() {
    for(FreeListEntry** freeList = mFreeLists.begin(); freeList != mFreeLists.end(); ++freeList) {
      while((*freeList)->getListSize() > 0) {
        *freeList = freeEntry(*freeList);
      }
    }
  }

  void* alloc(size_t numberElements) {
    #ifdef PING_DEBUG
    std::cout << "alloc function()" << std::endl;
    printf("------test test test----\n");
    printf("----numberElements = %zu\n", numberElements);
#endif
    FreeListEntry* & head = getFreeListHead(numberElements);
    #ifdef PING_DEBUG
    printf("head = %p\n", head);
    uint8_t * val = (uint8_t *)&head;
    printf("%d %d %d %d %d %d %d %d\n",val[0],val[1],val[2],val[3],val[4],val[5], val[6], val[7]);
    printf("head.getListSize = %zu\n", head->getListSize());
    #endif
    void* rawMemory;
    #ifdef PING_DEBUG
    uint8_t * val1 = (uint8_t *)&rawMemory;
    printf("rawMemory init = %p %p %p %p %p %p %p %p\n",val1[0],val1[1],val1[2],val1[3],val1[4],val1[5], val1[6], val1[7]);
    #endif
    if(head->getListSize() == 0) {
      ++mNumberAllocations;
      #ifdef PING_DEBUG
      uint8_t * val11 = (uint8_t *)&rawMemory;
      printf("ElementType size = %d, numberElements = %d\n", sizeof(ElementType), numberElements);
      printf("rawMemory before align = %p %p %p %p %p %p %p %p\n",val11[0],val11[1],val11[2],val11[3],val11[4],val11[5], val11[6], val11[7]);
      #endif
      int error = posix_memalign(&rawMemory, sizeof(ElementType), numberElements * sizeof(ElementType));
#ifdef PING_DEBUG
      uint8_t * val2 = (uint8_t *)&rawMemory;
      printf("rawMemory after align = %p %p %p %p %p %p %p %p\n",val2[0],val2[1],val2[2],val2[3],val2[4],val2[5], val2[6], val2[7]);
     #endif
      if(error != 0) {
        //"Got error on alignment"
        throw std::bad_alloc();
      }
    } else {
      rawMemory = reinterpret_cast<void*>(head);
      head = head->getNext();
    }
    #ifdef PING_DEBUG
    printf("raqMemory = %p\n", rawMemory);
 #endif
    return rawMemory;
  }

  void returnToPool(size_t numberElements, void* rawMemory) {
    FreeListEntry* & head = getFreeListHead(numberElements);
    if(head->getListSize() < SIZE_BEFORE_EVICTION_BEGIN_SIZE) {
      head = new (rawMemory) FreeListEntry(head);
    } else {
      free(rawMemory);
      ++mNumberFrees;
      while (head->getListSize() > EVICTION_END_SIZE) {
        head = freeEntry(head);
      }
    }
  }

  size_t getNumberAllocations() const {
    return mNumberAllocations;
  }

  size_t getNumberFrees() const {
    return mNumberFrees;
  }

private:
  FreeListEntry* freeEntry(FreeListEntry* head) {
    assert(head->getListSize() != 0u);
    FreeListEntry* next = head->getNext();
    free(head);
    ++mNumberFrees;
    return next;
  }

  FreeListEntry* & getFreeListHead(size_t numberElements) {
    return mFreeLists[numberElementsToFreeListId(numberElements)];
  }

  size_t numberElementsToFreeListId(size_t numberElements) {
    assert(numberElements > 0);
    assert(numberElements <= NUMBER_LISTS);

    return numberElements - 1;
  }
};

template<typename ElementType, size_t NUMBER_LISTS, size_t EVICTION_BEGIN_SIZE, size_t EVICTION_END_SIZE>
FreeListEntry MemoryPool<ElementType, NUMBER_LISTS, EVICTION_BEGIN_SIZE, EVICTION_END_SIZE>::TERMINATING_ENTRY {};

}}

#endif
