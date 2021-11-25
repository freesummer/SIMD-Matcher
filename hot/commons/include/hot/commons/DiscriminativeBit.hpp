#ifndef __HOT__COMMONS__DISCRIMINATIVE_BIT__
#define __HOT__COMMONS__DISCRIMINATIVE_BIT__

#include "hot/commons/include/hot/commons/Algorithms.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/OptionalValue.hpp"
//#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedChildPointerInterface.hpp"
//#include "hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedInterface.hpp"

#include <bitset>
extern bool flag_mask;

namespace hot { namespace commons {

constexpr uint8_t BYTE_WITH_MOST_SIGNIFICANT_BIT = 0b10000000;

/**
 * Describes a keys single bit by its position and its value
 */
struct DiscriminativeBit {
	uint16_t const mByteIndex;
	uint16_t const mByteRelativeBitIndex;
	uint16_t const mAbsoluteBitIndex;
	uint mValue;


public:
	inline DiscriminativeBit(uint16_t const significantByteIndex, uint8_t const existingByte, uint8_t const newKeyByte);
  inline DiscriminativeBit(uint16_t const significantByteIndex, uint8_t const existingByte, uint8_t const newKeyByte, uint8_t const maskByte);
	inline DiscriminativeBit(uint16_t const absoluteSignificantBitIndex, uint const newBitValue=1);

	inline uint8_t getExtractionByte() const;

private:
	static inline uint16_t getByteRelativeSignificantBitIndex(uint8_t const existingByte, uint8_t const newKeyByte);
  static inline uint16_t getByteRelativeSignificantBitIndex(uint8_t const existingByte, uint8_t const newKeyByte, uint8_t const newMaskByte);

};

inline DiscriminativeBit::DiscriminativeBit(uint16_t const significantByteIndex, uint8_t const existingByte, uint8_t const newKeyByte)
	: mByteIndex(significantByteIndex)
	, mByteRelativeBitIndex(getByteRelativeSignificantBitIndex(existingByte, newKeyByte))
	, mAbsoluteBitIndex(convertBytesToBits(mByteIndex) + mByteRelativeBitIndex)
	, mValue(((BYTE_WITH_MOST_SIGNIFICANT_BIT >> mByteRelativeBitIndex) & newKeyByte) != 0)
{
}

/*
 * Added this new function using in function: executeForDiffingKeys()
 * added maskByte for each byte, which passing from newMaskByte
 */
inline DiscriminativeBit::DiscriminativeBit(uint16_t const significantByteIndex, uint8_t const existingByte, uint8_t const newKeyByte, uint8_t const maskByte)
  : mByteIndex(significantByteIndex)
  , mByteRelativeBitIndex(getByteRelativeSignificantBitIndex(existingByte, newKeyByte, maskByte))
  , mAbsoluteBitIndex(convertBytesToBits(mByteIndex) + mByteRelativeBitIndex)
  , mValue(((BYTE_WITH_MOST_SIGNIFICANT_BIT >> mByteRelativeBitIndex) & newKeyByte) != 0)
{
}



inline DiscriminativeBit::DiscriminativeBit(uint16_t const absoluteSignificantBitIndex, uint const newBitValue)
	: mByteIndex(getByteIndex(absoluteSignificantBitIndex))
	, mByteRelativeBitIndex(bitPositionInByte(absoluteSignificantBitIndex))
	, mAbsoluteBitIndex(absoluteSignificantBitIndex)
	, mValue(newBitValue)
{
}

inline uint8_t DiscriminativeBit::getExtractionByte() const {
	return 1 << (7 - mByteRelativeBitIndex);
}

inline uint16_t DiscriminativeBit::getByteRelativeSignificantBitIndex(uint8_t const existingByte, uint8_t const newKeyByte) {
	uint32_t mismatchByteBitMask = existingByte ^ newKeyByte;
	return __builtin_clz(mismatchByteBitMask) - 24;
}

inline uint16_t DiscriminativeBit::getByteRelativeSignificantBitIndex(uint8_t const existingByte, uint8_t const newKeyByte, uint8_t const newMaskByte) {
  uint32_t mismatchByteBitMask = (existingByte ^ newKeyByte) & (~newMaskByte);
  /*
   * if the mismatchByteBitMask == 0, the non-wildcard part are the same, need to insert using my new method
   * else, the non-wildcard part are different, using the original way to insert
   */
  if (mismatchByteBitMask > 0) {
    return __builtin_clz(mismatchByteBitMask) - 24;
  } else {
    uint32_t tmp = uint32_t(newMaskByte);
#ifdef PING_DEBUG3
std::cout << "newMaskByte in 32 bit = " << std::bitset<32>(tmp) << std::endl;
#endif
    return __builtin_clz(tmp) - 24;
  }

}

/*check if the keys are different
 * return true: if different
 */
template<typename Operation> inline bool executeForDiffingKeys(uint8_t const* existingKey, uint8_t const* newKey, uint16_t keyLengthInBytes, Operation const & operation) {
   #ifdef PING_DEBUG3
  std::cout << "check if the keys are different:==executeForDiffingKeys function==" << std::endl;
  std::cout << "existingKey = " << std::bitset<64>(*existingKey) << ", newKey = " << std::bitset<64>(*newKey) << ", keyLengthInBytes = " << keyLengthInBytes << std::endl;
#endif

  for(size_t index = 0; index < keyLengthInBytes; ++index) {
     #ifdef PING_DEBUG3
    std::cout << "index = " << index << std::endl;
    #endif
		uint8_t newByte = newKey[index];
		uint8_t existingByte = existingKey[index];
    #ifdef PING_DEBUG3
    std::cout << "newByte = " << std::bitset<64>(newByte) << ", existingByte = " << std::bitset<64>(existingByte) << std::endl;
    #endif

    //  static_cast<uint16_t>(index): <.> is the new_type, return the index of type uint16_t
    if(existingByte != newByte) {
			operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte, newByte });
			return true;
		}
	}
	return false;
};

/*
 * static_cast<uint16_t>(index): <.> is the new_type, return the index of type uint16_t
 *
 */
template<typename Operation> inline bool executeForDiffingKeys(uint64_t mask, uint8_t const* existingKey, uint8_t const* newKey, uint16_t keyLengthInBytes, Operation const & operation) {
   #ifdef PING_DEBUG3
  std::cout << "-------New-----==executeForDiffingKeys function==" << std::endl;
#endif


 uint64_t newMask = __bswap_64(mask);

#ifdef PING_DEBUG3
std::cout << "newMask      = " << std::bitset<64>(newMask ) << std::endl;
#endif

 uint8_t *pp1 = (uint8_t *)&newMask;

  for(size_t index = 0; index < keyLengthInBytes; ++index) {
     #ifdef PING_DEBUG3
    std::cout << "index = " << index << std::endl;
    #endif
    uint8_t newByte = newKey[index];
    uint8_t existingByte = existingKey[index];
    uint8_t newMaskByte = pp1[index];
    #ifdef PING_DEBUG3
    std::cout << "newByte      = " << std::bitset<8>(newByte) << std::endl;
    std::cout << "existingByte = " << std::bitset<8>(existingByte) << std::endl;
    std::cout << "newMaskByte  = " << std::bitset<8>(newMaskByte) << std::endl;
#endif

    if( (existingByte | newMaskByte) != (newByte | newMaskByte) ) {
      operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte, newByte });
      return true;
    }
    if ( ((existingByte | newMaskByte) == (newByte | newMaskByte)) && newMaskByte > 0 ) {
      /*
       * if this byte partition mask > 0, even the non-wildcard are the same, we need to insert this new Rule has "*"
       * probably need to get some information here, to prepare for the next process: to find the affectedSubtree or the leafNode
       */
      flag_mask = true;
#ifdef PING_DEBUG3
std::cout << "-----flag_mask  = " << flag_mask << std::endl;
#endif
existingByte = existingByte | newMaskByte;
newByte = newByte | newMaskByte;
/*
 *  need to get the useful information to get affectedSubtree
 *  and insert this wildcard rule into all the entries under the afftectedSubtree
 */

#ifdef PING_DEBUG3
std::cout << "=================test test flag_mask======test test=============" << flag_mask << std::endl;
#endif

operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte, newByte, newMaskByte });

return true;

//      return false;
    }

//    if(existingByte != newByte) {
//      operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte, newByte });
//      return true;
//    }


  }
  return false;
};


/*
 * Added matchedMask argument, to consistent with the insertStack.....()
 *
 */


template<typename Operation> inline bool executeForDiffingKeys(uint64_t mask, uint8_t const* existingKey, uint64_t matchedMask, uint8_t const* newKey, uint16_t keyLengthInBytes, Operation const & operation) {
   #ifdef PING_DEBUG3
  std::cout << "---New----New-----==executeForDiffingKeys function==" << std::endl;
#endif

 uint64_t oldMask = __bswap_64(matchedMask);


 uint64_t newMask = __bswap_64(mask);

#ifdef PING_DEBUG3
std::cout << "newMask      = " << std::bitset<64>(newMask ) << std::endl;
std::cout << "oldMask      = " << std::bitset<64>(newMask ) << std::endl;
#endif

 uint8_t *pp1 = (uint8_t *)&newMask;
 uint8_t *pp2 = (uint8_t *)&oldMask;

  for(size_t index = 0; index < keyLengthInBytes; ++index) {
     #ifdef PING_DEBUG3
    std::cout << "index = " << index << std::endl;
    #endif
    uint8_t newByte = newKey[index];
    uint8_t existingByte = existingKey[index];
    uint8_t newMaskByte = pp1[index];
    uint8_t oldMaskByte = pp2[index];
    #ifdef PING_DEBUG3
    std::cout << "newByte      = " << std::bitset<8>(newByte) << std::endl;
    std::cout << "existingByte = " << std::bitset<8>(existingByte) << std::endl;
    std::cout << "newMaskByte  = " << std::bitset<8>(newMaskByte) << std::endl;
    std::cout << "oldMaskByte  = " << std::bitset<8>(oldMaskByte) << std::endl;
#endif

    if( (existingByte | oldMaskByte | newMaskByte) != (newByte | oldMaskByte | newMaskByte) ) {
      operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte | oldMaskByte | newMaskByte, newByte | oldMaskByte | newMaskByte });
      return true;
    }
    if ( (existingByte | oldMaskByte | newMaskByte) == (newByte | oldMaskByte | newMaskByte) && newMaskByte > 0 ) {
      /*
       * if this byte partition mask > 0, even the non-wildcard are the same, we need to insert this new Rule has "*"
       * probably need to get some information here, to prepare for the next process: to find the affectedSubtree or the leafNode
       */
      flag_mask = true;
#ifdef PING_DEBUG3
std::cout << "-----flag_mask  = " << flag_mask << std::endl;
#endif
existingByte = existingByte | newMaskByte;
newByte = newByte | newMaskByte;
/*
 *  need to get the useful information to get affectedSubtree
 *  and insert this wildcard rule into all the entries under the afftectedSubtree
 */

#ifdef PING_DEBUG3
std::cout << "=================test test flag_mask======test test=============" << flag_mask << std::endl;
#endif

operation(DiscriminativeBit {static_cast<uint16_t>(index), existingByte, newByte, newMaskByte });

return true;

    }

  }
  return false;
};


inline idx::contenthelpers::OptionalValue<DiscriminativeBit> getMismatchingBit(uint8_t const* existingKey, uint8_t const* newKey, uint16_t keyLengthInBytes) {
	for(size_t index = 0; index < keyLengthInBytes; ++index) {
		uint8_t newByte = newKey[index];
		uint8_t existingByte = existingKey[index];
		if(existingByte != newByte) {
      #ifdef PING_DEBUG
      std::cout << "getMismatchingBit" << index << std::endl;
      #endif
			return { true, DiscriminativeBit { static_cast<uint16_t>(index), existingByte, newByte } };
		}
	}
	return { false, { 0, 0, 0 }};
};

} }

#endif
