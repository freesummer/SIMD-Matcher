#ifndef __HOT__COMMONS__TWO_ENTRIES_NODE__
#define __HOT__COMMONS__TWO_ENTRIES_NODE__

#include <cstdint>

#include <hot/commons/include/hot/commons/SingleMaskPartialKeyMapping.hpp>
#include <hot/commons/include/hot/commons/BiNode.hpp>

namespace hot { namespace commons {

template<typename ChildPointerType, template <typename, typename> typename NodeTemplate> inline NodeTemplate<SingleMaskPartialKeyMapping, uint8_t>* createTwoEntriesNode(BiNode<ChildPointerType> const & binaryNode) {
  #ifdef PING_DEBUG3
  std::cout << "createTwoEntriesNode function: " << std::endl;
#endif
	constexpr uint16_t NUMBER_ENTRIES_IN_TWO_ENTRIES_NODE = 2u;
	NodeTemplate<SingleMaskPartialKeyMapping, uint8_t>* node =
		new (NUMBER_ENTRIES_IN_TWO_ENTRIES_NODE) NodeTemplate<SingleMaskPartialKeyMapping, uint8_t>(
			binaryNode.mHeight,
			NUMBER_ENTRIES_IN_TWO_ENTRIES_NODE,
			SingleMaskPartialKeyMapping { DiscriminativeBit { binaryNode.mDiscriminativeBitIndex } }
		);
#ifdef PING_DEBUG
  printf("node = %p\n", node);
//  uint8_t *val = (uint8_t*)&node;
//  std::cout << "new node = ";
//  printf("%p %p %p %p %p %p %p %p\n",val[0],val[1],val[2],val[3],val[4],val[5], val[6], val[7]);
//  printf("node.size = %d, node->mPartialKeys.size = %d\n", sizeof(node), sizeof(node->mPartialKeys));
#endif
	node->mPartialKeys.mEntries[0] = 0;
	node->mPartialKeys.mEntries[1] = 1;
	ChildPointerType* pointers = node->getPointers();
#ifdef PING_DEBUG
  printf("pointers = %p\n", pointers);
#endif
 #ifdef PING_DEBUG
  printf("pointers.size = %d\n", sizeof(pointers));
  uint8_t *val1 = (uint8_t*)&pointers;
  std::cout << "pointers = ";
  printf("%p %p %p %p %p %p %p %p\n",val1[0],val1[1],val1[2],val1[3],val1[4],val1[5], val1[6], val1[7]);
  #endif
  pointers[0] = binaryNode.mLeft;
	pointers[1] = binaryNode.mRight;
#ifdef PING_DEBUG
  printf("pointers[0] = %p, pointers[1] = %p\n", pointers[0], pointers[1]);
  printf("===========\n");
#endif
	return node;
};

} }

#endif
