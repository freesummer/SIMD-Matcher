#ifndef __HOT__COMMONS__SINGLE_MASK_PARTIAL_KEY_MAPPING_HPP___
#define __HOT__COMMONS__SINGLE_MASK_PARTIAL_KEY_MAPPING_HPP___

#include <immintrin.h>

#include <cstring>
#include <set>
#include <bitset>

#include "hot/commons/include/hot/commons/Algorithms.hpp"
#include "hot/commons/include/hot/commons/DiscriminativeBit.hpp"

#include "hot/commons/include/hot/commons/PartialKeyMappingBase.hpp"
#include "hot/commons/include/hot/commons/SingleMaskPartialKeyMappingInterface.hpp"
#include "hot/commons/include/hot/commons/MultiMaskPartialKeyMapping.hpp"

#include "idx/content-helpers/include/idx/contenthelpers/KeyUtilities.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/TidConverters.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/ContentEquals.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/KeyComparator.hpp"
#include "idx/content-helpers/include/idx/contenthelpers/OptionalValue.hpp"


extern uint64_t extract_mask;

namespace hot { namespace commons {

constexpr uint64_t SUCCESSIVE_EXTRACTION_MASK_WITH_HIGHEST_BIT_SET = 1ul << 63;

inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(SingleMaskPartialKeyMapping const & src)
  : PartialKeyMappingBase()
{
  //  store 128-bit integer data from the second argument into the first argument
  //  load 128-bit of integer data from memory address &src
  _mm_storeu_si128(reinterpret_cast<__m128i*>(this), _mm_loadu_si128(reinterpret_cast<__m128i const *>(&src)));
}

inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(DiscriminativeBit const & discriminativeBit)
  : PartialKeyMappingBase(discriminativeBit.mAbsoluteBitIndex, discriminativeBit.mAbsoluteBitIndex),
    mOffsetInBytes(getSuccesiveByteOffsetForMostRightByte(discriminativeBit.mByteIndex)),
    mSuccessiveExtractionMask(getSuccessiveMaskForBit(discriminativeBit.mByteIndex, discriminativeBit.mByteRelativeBitIndex)) {
  assert(mOffsetInBytes < 255);
}

inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(
    uint8_t const * extractionBytePositions,
    uint8_t const * extractionByteData,
    uint32_t const extractionBytesUsedMask,
    uint16_t const mostSignificantBitIndex,
    uint16_t const leastSignificantBitIndex
    ) : PartialKeyMappingBase(mostSignificantBitIndex, leastSignificantBitIndex),
  mOffsetInBytes(getSuccesiveByteOffsetForLeastSignificantBitIndex(leastSignificantBitIndex)),
  mSuccessiveExtractionMask(getSuccessiveExtractionMaskFromRandomBytes(extractionBytePositions, extractionByteData, extractionBytesUsedMask, mOffsetInBytes))
{
  assert(mOffsetInBytes < 255);
}


inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(
    SingleMaskPartialKeyMapping const & existing,
    DiscriminativeBit const & discriminatingBit
    ) : PartialKeyMappingBase(existing, discriminatingBit),
  mOffsetInBytes(getSuccesiveByteOffsetForLeastSignificantBitIndex(mLeastSignificantDiscriminativeBitIndex)),
  mSuccessiveExtractionMask(
    getSuccessiveMaskForBit(discriminatingBit.mByteIndex, discriminatingBit.mByteRelativeBitIndex)
    | (existing.mSuccessiveExtractionMask >> (convertBytesToBits(mOffsetInBytes - existing.mOffsetInBytes)))
    )
{
  assert(mOffsetInBytes < 255);
}



inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(
    SingleMaskPartialKeyMapping const & existing,
    uint32_t const & maskBitsNeeded
    ) : SingleMaskPartialKeyMapping(existing, existing.getSuccessiveMaskForMask(maskBitsNeeded))
{
  assert(_mm_popcnt_u32(maskBitsNeeded) >= 1);
  assert(mOffsetInBytes < 255);
}

inline uint16_t SingleMaskPartialKeyMapping::calculateNumberBitsUsed() const {
#ifdef PING_DEBUG3
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
  printf("SingleMaskPartialKeyMapping::calculateNumberBitsUsed() = %lld\n", _mm_popcnt_u64(mSuccessiveExtractionMask));
#endif
  return _mm_popcnt_u64(mSuccessiveExtractionMask);
}

template<typename PartialKeyType> inline PartialKeyType
SingleMaskPartialKeyMapping::getPrefixBitsMask(DiscriminativeBit const &significantKeyInformation) const {
#ifdef PING_DEBUG3
  printf("SingleMaskPartialKeyMapping::getPrefixBitsMask function () begins----\n");
  std::cout << "significantKeyInformation.mByteIndex = " << significantKeyInformation.mByteIndex << std::endl;
  std::cout << "mOffsetInBytes = " << mOffsetInBytes << std::endl;
#endif
  int relativeMissmatchingByteIndex = significantKeyInformation.mByteIndex - mOffsetInBytes;
#ifdef PING_DEBUG3
  printf("relativeMissmatchingByteIndex:significantKeyInformation.mByteIndex - mOffsetInBytes = %d\n", relativeMissmatchingByteIndex);
#endif
  //Extracts a bit masks corresponding to 111111111|00000000 with the first zero bit marking the missmatching bit index.

  //PAPER describe that little endian byte order must be respected.
  //due to little endian encoding inside the byte the shift direction is reversed
  //it has the form 1110000 111111111....
  uint64_t singleBytePrefixMask = ((UINT64_MAX >> significantKeyInformation.mByteRelativeBitIndex) ^ (UINT64_MAX << 56)) * (relativeMissmatchingByteIndex >= 0);
#ifdef PING_DEBUG3
  std::cout << "singleBytePrefixMask = " << std::bitset<64>(singleBytePrefixMask) << std::endl;
#endif
  //mask where highest byte (most right byte in little endian) is set to the deleted mask
  //results ins a mask like this 010111|1111111111111111...... where the pipe masks the end of the first byte
  //in the next step this mask is moved to the right to mask the prefix byte (in little endian move high byte to the right actually means moving high byte to the left hence turning the most right bytes 0 (after the prefix))
  uint64_t subtreeSuccesiveBytesMask = relativeMissmatchingByteIndex > 7 ? UINT64_MAX : (singleBytePrefixMask >> ((7 - relativeMissmatchingByteIndex) * 8));
#ifdef PING_DEBUG3
  std::cout << "subtreeSuccesiveBytesMask = " << std::bitset<64>(subtreeSuccesiveBytesMask) << std::endl;
#endif
  return extractMaskFromSuccessiveBytes(subtreeSuccesiveBytesMask);
}

template<typename Operation> inline auto SingleMaskPartialKeyMapping::insert(DiscriminativeBit const & discriminativeBit, Operation const & operation) const {
#ifdef PING_DEBUG3
  printf("SingleMaskPartialKeyMapping::insert function ()----begins\n");
  printf("mMostSignificantDiscriminativeBitIndex = %d, mLeastSignificantDiscriminativeBitIndex = %d\n", mMostSignificantDiscriminativeBitIndex, mLeastSignificantDiscriminativeBitIndex);
#endif
  bool isSingleMaskPartialKeyMapping =
      ((static_cast<int>(discriminativeBit.mByteIndex - getByteIndex(mMostSignificantDiscriminativeBitIndex))) < 8)
      & ((static_cast<int>(getByteIndex(mLeastSignificantDiscriminativeBitIndex) - discriminativeBit.mByteIndex)) < 8);
#ifdef PING_DEBUG3
  printf("isSingleMaskPartialKeyMapping = %d\n", isSingleMaskPartialKeyMapping);
#endif
  return isSingleMaskPartialKeyMapping
      ? operation(SingleMaskPartialKeyMapping { *this, discriminativeBit })
      : (hasUnusedBytes()
         ? operation(MultiMaskPartialKeyMapping<1u> { *this, discriminativeBit })
         : operation(MultiMaskPartialKeyMapping<2u> { *this, discriminativeBit })
           );
}

template<typename Operation> inline auto SingleMaskPartialKeyMapping::extract(uint32_t bitsUsed, Operation const & operation) const {
  assert(_mm_popcnt_u32(bitsUsed) >= 1);
  return operation(SingleMaskPartialKeyMapping { *this, bitsUsed } );
}

template<typename Operation> inline auto SingleMaskPartialKeyMapping::executeWithCorrectMaskAndDiscriminativeBitsRepresentation(Operation const & operation) const {
  assert(getMaximumMaskByteIndex(calculateNumberBitsUsed()) <= 3);
  switch(getMaximumMaskByteIndex(calculateNumberBitsUsed())) {
  case 0:
    return operation(*this, static_cast<uint8_t>(UINT8_MAX));
  case 1:
    return operation(*this, static_cast<uint16_t>(UINT16_MAX));
  default: //case 2 + 3
    return operation(*this, static_cast<uint32_t>(UINT32_MAX));
  }
}

inline bool SingleMaskPartialKeyMapping::hasUnusedBytes() const {
#ifdef PING_DEBUG3
  printf("hasUnusedBytes = %d\n", getUsedBytesMask() != UINT8_MAX);
#endif
  return getUsedBytesMask() != UINT8_MAX;
}

inline uint32_t SingleMaskPartialKeyMapping::getUsedBytesMask() const {
  __m64 extractionMaskRegister = getRegister();
  return _mm_movemask_pi8(_mm_cmpeq_pi8(extractionMaskRegister, _mm_setzero_si64())) ^ UINT8_MAX;
}

inline uint32_t SingleMaskPartialKeyMapping::getByteOffset() const {
  return mOffsetInBytes;
}


inline uint8_t SingleMaskPartialKeyMapping::getExtractionByte(unsigned int byteIndex) const {
  return reinterpret_cast<uint8_t const *>(&mSuccessiveExtractionMask)[byteIndex];
}

inline uint8_t SingleMaskPartialKeyMapping::getExtractionBytePosition(unsigned int byteIndex) const {
  return byteIndex + mOffsetInBytes;
}

inline uint32_t SingleMaskPartialKeyMapping::getMaskForHighestBit() const {
  return extractMaskFromSuccessiveBytes(getSuccessiveMaskForAbsoluteBitPosition(mMostSignificantDiscriminativeBitIndex));
}

inline uint32_t SingleMaskPartialKeyMapping::getMaskFor(DiscriminativeBit const & discriminativeBit) const {
#ifdef PING_DEBUG3
  printf("getMaskFor function () #2 begin----\n");
#endif
  return extractMaskFromSuccessiveBytes(getSuccessiveMaskForBit(discriminativeBit.mByteIndex, discriminativeBit.mByteRelativeBitIndex));
}

inline uint32_t SingleMaskPartialKeyMapping::getAllMaskBits() const {
#ifdef PING_DEBUG3
  printf("--SingleMaskPartialKeyMapping::getAllMaskBits() function starts....\n");
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
  std::cout << "getAllMaskBits() function ends....return _pext_u64(mSuccessiveExtractionMask, mSuccessiveExtractionMask) = " << std::bitset<64>(_pext_u64(mSuccessiveExtractionMask, mSuccessiveExtractionMask)) << std::endl;
#endif
  return _pext_u64(mSuccessiveExtractionMask, mSuccessiveExtractionMask);
}

inline __attribute__((always_inline)) uint32_t SingleMaskPartialKeyMapping::extractMask(uint8_t const * keyBytes) const {
#ifdef PING_DEBUG3
  printf("--SingleMaskPartialKeyMapping::extractMask function () starts----\n");
  std::cout << "mOffsetInBytes = " << mOffsetInBytes << ", keyBytes = " <<  std::bitset<64>(*keyBytes) << std::endl;
#endif
  return  extractMaskFromSuccessiveBytes(*reinterpret_cast<uint64_t const*>(keyBytes + mOffsetInBytes));
}


inline __attribute__((always_inline)) uint32_t SingleMaskPartialKeyMapping::extractMask(uint8_t const * keyBytes, uint64_t mask) const {
#ifdef PING_DEBUG3
  printf("--New SingleMaskPartialKeyMapping::extractMask function () starts--added mask argument--\n");
  std::cout << "mOffsetInBytes = " << mOffsetInBytes << ", keyBytes = " <<  std::bitset<64>(*keyBytes) << std::endl;
#endif
  return  extractMaskFromSuccessiveBytes(*reinterpret_cast<uint64_t const*>(keyBytes + mOffsetInBytes), mask);
}

inline std::array<uint8_t, 256> SingleMaskPartialKeyMapping::createIntermediateKeyWithOnlySignificantBitsSet() const {
  std::array<uint8_t, 256> intermediateKey;
  std::memset(intermediateKey.data(), 0, 256);
  std::memmove(intermediateKey.data() + mOffsetInBytes, &mSuccessiveExtractionMask, sizeof(mSuccessiveExtractionMask));
  return intermediateKey;
};

inline SingleMaskPartialKeyMapping::SingleMaskPartialKeyMapping(
    SingleMaskPartialKeyMapping const & existing, uint64_t const newExtractionMaskWithSameOffset
    ) : PartialKeyMappingBase(
          convertBytesToBits(existing.mOffsetInBytes) + calculateRelativeMostSignificantBitIndex(newExtractionMaskWithSameOffset),
          convertBytesToBits(existing.mOffsetInBytes) + calculateRelativeLeastSignificantBitIndex(newExtractionMaskWithSameOffset)
          ),
  mOffsetInBytes(static_cast<uint32_t>(getSuccesiveByteOffsetForLeastSignificantBitIndex(mLeastSignificantDiscriminativeBitIndex))),
  mSuccessiveExtractionMask(newExtractionMaskWithSameOffset << (convertBytesToBits(existing.mOffsetInBytes - mOffsetInBytes)))
{
}

inline __attribute__((always_inline)) uint32_t SingleMaskPartialKeyMapping::extractMaskFromSuccessiveBytes(uint64_t const inputMask) const {
#ifdef PING_DEBUG3
  printf("extractMaskFromSuccessiveBytes function () ---begins\n");
  std::cout << "inputMask = " << std::bitset<64>(inputMask) << std::endl;
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
  std::cout << "extractMaskFromSuccessiveBytes function () return _pext_u64(inputMask, mSuccessiveExtractionMask) function rst = " << std::bitset<64>(_pext_u64(inputMask, mSuccessiveExtractionMask)) << std::endl;
  printf("--SingleMaskPartialKeyMapping::extractMask function () ends===----\n");
#endif


#ifdef PING_DEBUG3
  uint32_t rst = static_cast<uint32_t>( _pext_u64(inputMask, mSuccessiveExtractionMask));
  std::cout << "rst extract = " << std::bitset<32>(rst) << std::endl;
#endif

  return static_cast<uint32_t>( _pext_u64(inputMask, mSuccessiveExtractionMask));
}


/*
 * added mask argument into this function
 * pass the shr_count global parameter out to other function
 *
 */

inline __attribute__((always_inline)) uint32_t SingleMaskPartialKeyMapping::extractMaskFromSuccessiveBytes(uint64_t const inputMask, uint64_t mask) const {
#ifdef PING_DEBUG3
  printf("New extractMaskFromSuccessiveBytes function----added mask argument () ---begins\n");
  std::cout << "inputMask = " << std::bitset<64>(inputMask) << std::endl;
  std::cout << "Mask = " << std::bitset<64>(mask) << std::endl;
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
  std::cout << "extractMaskFromSuccessiveBytes function () return _pext_u64(inputMask, mSuccessiveExtractionMask) function rst = " << std::bitset<64>(_pext_u64(inputMask, mSuccessiveExtractionMask)) << std::endl;
  printf("----New--SingleMaskPartialKeyMapping::extractMask function () ends===----\n");
#endif
  /*
   * get the lowest bit, which can indicate the left kid or right kid
   * if the lowest bit is 0, then left kid
   * otherwise, right kid
   */

  /*
   * Get the shift scale: bb = mSuccessiveExtractionMask & mask =====>> get the number of bit "1"
   * Using _mm_popcnt_u64(uint64_t bb) to get the count
   *
   */

  uint32_t rst = static_cast<uint32_t>( _pext_u64(inputMask, mSuccessiveExtractionMask));

#ifdef PING_DEBUG3
  std::cout << "rst extract = " << std::bitset<32>(rst) << std::endl;
#endif

  if (mask == 0) {
    extract_mask = 0;
#ifdef PING_DEBUG3
    printf("--------mask = %lu,----extract_mask = %lu\n", mask, extract_mask);
#endif
    return rst;
  } else {
    //  need to change the format to the same with the "mSuccessiveExtractionMask"
//    uint64_t newMask = getReverseBitForBytes(mask);
    uint64_t newMask = __bswap_64(mask);

#ifdef PING_DEBUG3
    std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
    std::cout << "newMask =                   " << std::bitset<64>(newMask) << std::endl;
    std::cout << "mSucce..... ^     newMask = " << std::bitset<64>(mSuccessiveExtractionMask ^ newMask) << std::endl;
#endif


    extract_mask = _pext_u64((mSuccessiveExtractionMask ^ newMask), mSuccessiveExtractionMask);




#ifdef PING_DEBUG3
    std::cout << "------extract_mask = _pext_u64((mSuccessiveExtractionMask ^ newMask), mSuccessiveExtractionMask)----" << std::bitset<64>(extract_mask) << std::endl;
#endif

//        rst = rst & extract_mask;

    rst = static_cast<uint32_t>( _pext_u64(rst, extract_mask) );

#ifdef PING_DEBUG3
    std::cout << "after bits extraction from wildcard bit positions =====>> rst extract = " << std::bitset<32>(rst) << std::endl;
#endif

    return rst;
  }


}



inline __m64 SingleMaskPartialKeyMapping::getRegister() const {
  return _mm_cvtsi64_m64(mSuccessiveExtractionMask);
}

inline uint64_t SingleMaskPartialKeyMapping::getSuccessiveMaskForBit(uint const bytePosition, uint const byteRelativeBitPosition) const {
#ifdef PING_DEBUG3
  printf("getSuccessiveMaskForBit function () ---begin\n");
  printf("bytePosition = %d, byteRelativeBitPosition = %d\n", bytePosition, byteRelativeBitPosition);
  printf("mOffsetInbytes = %d, function return: 1 << middle_rst = %d\n", mOffsetInBytes, convertToIndexOfOtherEndiness(bytePosition - mOffsetInBytes, byteRelativeBitPosition));
#endif
  return 1ul << convertToIndexOfOtherEndiness(bytePosition - mOffsetInBytes, byteRelativeBitPosition);
}

inline uint SingleMaskPartialKeyMapping::convertToIndexOfOtherEndiness(uint const maskRelativeBytePosition, uint const byteRelativeBitPosition) {
#ifdef PING_DEBUG3
  printf("convertToIndexOfOtherEndiness function () ---begin\n");
  printf("maskRelativeBytePosition = %d, byteRelativeBitPosition = %d\n", maskRelativeBytePosition, byteRelativeBitPosition);
  printf("function return: (maskRelativeBytePosition * 8) + 7 - byteRelativeBitPosition = %d\n", (maskRelativeBytePosition * 8) + 7 - byteRelativeBitPosition);
#endif
  return (maskRelativeBytePosition * 8) + 7 - byteRelativeBitPosition;
}

inline uint64_t SingleMaskPartialKeyMapping::getSuccessiveMaskForAbsoluteBitPosition(uint absoluteBitPosition) const {
  return getSuccessiveMaskForBit(getByteIndex(absoluteBitPosition), bitPositionInByte(absoluteBitPosition));
}

inline uint64_t SingleMaskPartialKeyMapping::getSuccessiveMaskForMask(uint32_t const mask) const {
#ifdef PING_DEBUG3
  printf("SingleMaskPartialKeyMapping::getSuccessiveMaskForMask function ()----begins---\n");
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
#endif
  return _pdep_u64(mask, mSuccessiveExtractionMask);
}

inline uint SingleMaskPartialKeyMapping::getSuccesiveByteOffsetForLeastSignificantBitIndex(uint leastSignificantBitIndex) {
  return getSuccesiveByteOffsetForMostRightByte(getByteIndex(leastSignificantBitIndex));
}

inline uint16_t SingleMaskPartialKeyMapping::calculateRelativeMostSignificantBitIndex(uint64_t rawExtractionMask) {
  assert(rawExtractionMask != 0);
  uint64_t reverseMask = __builtin_bswap64(rawExtractionMask);
  return __lzcnt64(reverseMask);
}

inline uint16_t SingleMaskPartialKeyMapping::calculateRelativeLeastSignificantBitIndex(uint64_t rawExtractionMask) {
  assert(rawExtractionMask != 0);
  return 63 - __tzcnt_u64(__builtin_bswap64(rawExtractionMask));
}

inline uint64_t SingleMaskPartialKeyMapping::getSuccessiveExtractionMaskFromRandomBytes(
    uint8_t const * extractionBytePositions,
    uint8_t const * extractionByteData,
    uint32_t extractionBytesUsedMask,
    uint32_t const offsetInBytes
    ) {
  uint64_t successiveExtractionMask = 0ul;
  uint8_t* successiveExtractionBytes = reinterpret_cast<uint8_t*>(&successiveExtractionMask);
  while(extractionBytesUsedMask > 0) {
    uint extractionByteIndex = __tzcnt_u32(extractionBytesUsedMask);
    uint targetExtractionBytePosition = extractionBytePositions[extractionByteIndex] - offsetInBytes;
    successiveExtractionBytes[targetExtractionBytePosition] = extractionByteData[extractionByteIndex];
    extractionBytesUsedMask = _blsr_u32(extractionBytesUsedMask);
  }
  return successiveExtractionMask;
}

template<typename PartialKeyType> inline PartialKeyType SingleMaskPartialKeyMapping::getMostSignifikantMaskBit(PartialKeyType mask) const {
#ifdef PING_DEBUG3
  printf("SingleMaskPartialKeyMapping::getMostSignifikantMaskBit function () ----begins----\n");
  std::cout << "mask = " << std::bitset<32>(mask) << std::endl;
#endif
  uint64_t correspondingSuccessiveExtractionMask = getSuccessiveMaskForMask(mask);
#ifdef PING_DEBUG3
  std::cout << "_pedp_64(mask, mSuccessiveExtractionMask) [since each entry may not involve the whole mask DB bits] generates: " << std::endl;
  std::cout << "correspondingSuccessiveExtractionMask = " << std::bitset<64>(correspondingSuccessiveExtractionMask) << std::endl;
#endif
  unsigned int byteShiftOffset = __tzcnt_u64(correspondingSuccessiveExtractionMask) & (~0b111);
#ifdef PING_DEBUG3
  printf("byteShiftOffset = %d\n", byteShiftOffset);
#endif
  uint64_t mostSignificantExtractionByteMask = correspondingSuccessiveExtractionMask & (static_cast<uint64_t>(UINT8_MAX) << byteShiftOffset); //pad -> remove relative bit index
#ifdef PING_DEBUG3
  std::cout << "mostSignificantExtractionByteMask = " << std::bitset<64>(mostSignificantExtractionByteMask) << std::endl;
#endif
  uint64_t extractionMaskWithOnlyMostSignificanMaskBitSet =  SUCCESSIVE_EXTRACTION_MASK_WITH_HIGHEST_BIT_SET >> _lzcnt_u64(mostSignificantExtractionByteMask);
#ifdef PING_DEBUG3
  std::cout << "extractionMaskWithOnlyMostSignificanMaskBitSet = " << std::bitset<64>(extractionMaskWithOnlyMostSignificanMaskBitSet) << std::endl;
#endif
  return extractMaskFromSuccessiveBytes(extractionMaskWithOnlyMostSignificanMaskBitSet);
}

inline uint16_t SingleMaskPartialKeyMapping::getLeastSignificantBitIndex(uint32_t partialKey) const {
  return convertBytesToBits(mOffsetInBytes) + calculateRelativeLeastSignificantBitIndex(getSuccessiveMaskForMask(partialKey));
}

inline std::set<uint16_t> SingleMaskPartialKeyMapping::getDiscriminativeBits() const {
#ifdef PING_DEBUG3
  printf("----------SingleMaskPartialKeyMapping::getDiscriminativeBits() function starts.......\n");
  std::cout << "mSuccessiveExtractionMask = " << std::bitset<64>(mSuccessiveExtractionMask) << std::endl;
#endif
  uint64_t swapedExtractionMask = __builtin_bswap64(mSuccessiveExtractionMask);
#ifdef PING_DEBUG3
  std::cout << "swapedExtractionMask = " << std::bitset<64>(swapedExtractionMask) << std::endl;
#endif
  std::set<uint16_t> extractionBits;

  while(swapedExtractionMask != 0) {
    uint isZero = swapedExtractionMask == 0;
    uint notIsZero = 1 - isZero;

    uint16_t bitIndex = (isZero * 63) + (notIsZero * __lzcnt64(swapedExtractionMask));
    uint64_t extractionBit = (1ul << 63) >> bitIndex;
#ifdef PING_DEBUG3
    std::cout << "bitIndex = " << bitIndex << std::endl;
    std::cout << "extractionBit = " << std::bitset<64>(extractionBit) << std::endl;
#endif

    swapedExtractionMask &= (~extractionBit);

#ifdef PING_DEBUG3
    std::cout << "swapedExtractionMask &= (~extractionBit) = " << std::bitset<64>(swapedExtractionMask) << std::endl;
    printf("mOffsetInBytes = %d\n", mOffsetInBytes);
#endif

    extractionBits.insert(convertBytesToBits(mOffsetInBytes) + bitIndex);
  }

#ifdef PING_DEBUG3
  std::cout << "extractionBits.size = " << extractionBits.size() << std::endl;
  for (std::set<uint16_t>::iterator m = extractionBits.begin(); m!= extractionBits.end(); m++) {
    std::cout << *m << std::endl;
  }

  printf("----------SingleMaskPartialKeyMapping::getDiscriminativeBits() function ---------end......\n");
#endif

  return extractionBits;
}

} }

#endif
