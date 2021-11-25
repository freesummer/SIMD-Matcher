#ifndef __HOT__COMMONS__ALGORITHMS__
#define __HOT__COMMONS__ALGORITHMS__

#include <cassert>
#include <bitset>
#include <immintrin.h>
#include <iostream>


namespace hot { namespace commons {

inline uint32_t getBytesUsedInExtractionMask(uint64_t successiveExtractionMask) {
	uint32_t const unsetBytes = _mm_movemask_pi8(_mm_cmpeq_pi8(_mm_and_si64(_mm_set_pi64x(successiveExtractionMask), _mm_set_pi64x(UINT64_MAX)), _mm_setzero_si64()));
	//8 - numberUnsetBytes
	return 8 - _mm_popcnt_u32(unsetBytes);
}

inline uint16_t getMaximumMaskByteIndex(uint16_t bitsUsed) {
	return (bitsUsed - 1)/8;
}

template<uint numberExtractionMasks> inline std::array<uint64_t, numberExtractionMasks> getUsedExtractionBitsForMask(uint32_t usedBits, uint64_t  const * extractionMask);

template<uint numberExtractionMasks> inline __m256i extractionMaskToRegister(std::array<uint64_t, numberExtractionMasks> const & extractionData);

template<> inline __m256i extractionMaskToRegister<1>(std::array<uint64_t, 1> const & extractionData) {
	return _mm256_set_epi64x(extractionData[0], 0ul, 0ul, 0ul);
};

template<> inline __m256i extractionMaskToRegister<2>(std::array<uint64_t, 2> const & extractionData) {
	return _mm256_set_epi64x(extractionData[0], extractionData[1], 0ul, 0ul);
}

template<> inline __m256i extractionMaskToRegister<4>(std::array<uint64_t, 4> const & extractionData) {
	return _mm256_loadu_si256(reinterpret_cast<__m256i const *>(extractionData.data()));
}

template<size_t numberBytes> inline std::array<uint8_t, numberBytes>  extractSuccesiveFromRandomBytes(uint8_t const * bytes, uint8_t const * bytePositions) {
	std::array<uint8_t, numberBytes> succesiveBytes;
	for(uint i=0; i < numberBytes; ++i) {
		succesiveBytes[i] = bytes[bytePositions[i]];
	}
	return std::move(succesiveBytes);
}

/**
 * @brief reverse_byte
 * @param x
 * @return reverse the bit in a byte, put the reverse value to the table
 * since 8-bit has 256 different values, so the table size = 256
 * the value from 0 ~ 255, like hard coded
 */

unsigned char reverse_byte(unsigned char x)
{
  static const unsigned char table[] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
  };
  return table[x];
}


void serialise_64bit(unsigned char dest[8], uint64_t n)
{
  dest[0] = (n >> 56) & 0xff;
  dest[1] = (n >> 48) & 0xff;
  dest[2] = (n >> 40) & 0xff;
  dest[3] = (n >> 32) & 0xff;
  dest[4] = (n >> 24) & 0xff;
  dest[5] = (n >> 16) & 0xff;
  dest[6] = (n >>  8) & 0xff;
  dest[7] = (n >>  0) & 0xff;

//  for (int i = 0; i < 8; i++) {
//    cout << "dest_" << i << " = " << bitset<8>(dest[i]) << endl;
//  }

}

/**
 * New function: reverse bit in a byte for a uint64_t type
 * @param value: the uint64_t value: 63 62 61 60 59 58 ........8 7 6 5 4 3 2 1 0
 * @return 8-bytes, for each byte, reverse the 8-bits in these 8 bytes
 * 56 57 58 59 60 61 62 63 | 48 49 50 51 52 53 54 55..........0 1 2 3 4 5 6 7
 *
 */

inline uint64_t getReverseBitForBytes(uint64_t original) {

  unsigned char dest[8] = {};
  serialise_64bit(dest, original);
  unsigned char cc[8] = {};
  for (int i = 0; i < 8; i++) {
    cc[i] = reverse_byte(dest[i]);
  }

  uint64_t rst = 0;
  for (int i = 0; i < 8; ++i) {
      rst = rst | (((uint64_t)cc[i]) << (8 * (7 - i)));
  }

  return rst;

}





/**
 * New function: change the value based on the mask part
 *
 * @param value: the old value, mask: the wildcard postions
 * @return the new value
 */

inline uint64_t getNewValue(uint64_t value, uint64_t mask) {
  return ( (value & (~mask)) | ((value & mask) ^ mask) );
}



/**
 * Given a bitindex this function returns its corresponding byte index
 *
 * @param bitIndex the bit index to convert to byte Level
 * @return the byte index
 */
inline unsigned int getByteIndex(unsigned int bitIndex) {
	return bitIndex/8;
}


/**
 * gets the number of bytes needed to represent the successive bytes from (inclusive) the byte containing
 * the mostSignificantBitIndex until (inclusive) the byte containing the leastSignificantBitIndex
 *
 * @param mostSignificantBitIndex the index of the most significant bit
 * @param leastSignificantBitIndex the index of the least significant bit
 * @return the size of the range in bytes
 */
inline uint getByteRangeSize(uint mostSignificantBitIndex, uint leastSignificantBitIndex) {
	return getByteIndex(leastSignificantBitIndex) - getByteIndex(mostSignificantBitIndex);
}

constexpr inline uint16_t convertBytesToBits(uint16_t const byteIndex) {
	return byteIndex * 8;
}

constexpr inline uint16_t bitPositionInByte(uint16_t const absolutBitPosition) {
	return absolutBitPosition % 8;
}

constexpr uint64_t HIGHEST_UINT64_BIT = (1ul << 63);

inline uint getSuccesiveByteOffsetForMostRightByte(uint mostRightByte) {
	return std::max(0, ((int) mostRightByte) - 7);
}

inline bool isNoMissmatch(std::pair<uint8_t const*, uint8_t const*> const & missmatch, uint8_t const* key1, uint8_t const* key2, size_t keyLength)  {
	return missmatch.first == (key1 + keyLength) && missmatch.second == (key2 + keyLength);
}

inline bool isBitSet(uint8_t const * existingRawKey, uint16_t const mAbsoluteBitIndex) {
	return (existingRawKey[getByteIndex(mAbsoluteBitIndex)] & (0b10000000 >> bitPositionInByte(mAbsoluteBitIndex))) > 0;
}


inline uint16_t getLeastSignificantBitIndexInByte(uint8_t byte) {
	return (7 - _tzcnt_u32(byte));
}

inline uint16_t getMostSignificantBitIndexInByte(uint8_t byte) {
	assert(byte > 0);
	return _lzcnt_u32(byte) - 24;
}

inline __attribute__((always_inline)) int getMostSignificantBitIndex(uint32_t number) {
	int msb;
	asm("bsr %1,%0" : "=r"(msb) : "r"(number));
	return msb;
}

inline __attribute__((always_inline)) int getMostSignificantBitIndex_64(uint64_t number) {
  int msb;
  msb = 63-__builtin_clzll(number);
  return msb;
}





} }

#endif
