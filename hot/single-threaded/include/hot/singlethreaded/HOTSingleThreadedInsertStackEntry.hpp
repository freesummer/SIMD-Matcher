#ifndef __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_INSERT_STACK_ENTRY___
#define __HOT__SINGLE_THREADED__HOT_SINGLE_THREADED_INSERT_STACK_ENTRY___

#include <hot/commons/include/hot/commons/SearchResultForInsert.hpp>

#include "HOTSingleThreadedChildPointer.hpp"



namespace hot { namespace singlethreaded {

struct HOTSingleThreadedInsertStackEntry {
	HOTSingleThreadedChildPointer *mChildPointer;
	hot::commons::SearchResultForInsert mSearchResultForInsert;

	inline void initLeaf(HOTSingleThreadedChildPointer * childPointer) {
    #ifdef PING_DEBUG
    std::cout << "-----initLeaf function:---initialize the HOTSingleThreadedChildPointer * array--- " << std::endl;
#endif
		mChildPointer = childPointer;
    *mChildPointer = *childPointer;

		//important for finding the correct depth!!
		mSearchResultForInsert.mMostSignificantBitIndex = UINT16_MAX;
    #ifdef PING_DEBUG
    std::cout << "--mChildPointer = childPointer---mMostSignificantBitIndex = UINT16_MAX---initLeaf function ---ends" << std::endl;
    #endif
	}

  inline void initLeaf(HOTSingleThreadedChildPointer * childPointer, uint64_t mask) {
    #ifdef PING_DEBUG
    std::cout << "initLeaf function with mask argument: " << std::endl;
#endif
    mChildPointer = childPointer;
    mChildPointer->mMask = mask;
    //important for finding the correct depth!!
    mSearchResultForInsert.mMostSignificantBitIndex = UINT16_MAX;
    #ifdef PING_DEBUG
    std::cout << "--mChildPointer = childPointer---mMostSignificantBitIndex = UINT16_MAX---initLeaf function ---ends" << std::endl;
    #endif
  }

	//PERFORMANCE this must be uninitialized
	inline HOTSingleThreadedInsertStackEntry() {
	}
};

} }

#endif
