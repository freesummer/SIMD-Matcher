#ifndef __HOT__COMMONS__SPARSE_PARTIAL_KEYS__
#define __HOT__COMMONS__SPARSE_PARTIAL_KEYS__

#include <immintrin.h>

#include <bitset>
#include <cassert>
#include <cstdint>

#include <hot/commons/include/hot/commons/Algorithms.hpp>
#include <hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedNodeBaseInterface.hpp>

extern uint64_t extract_mask;
extern uint32_t wildcard_affected_entry_mask;

namespace hot { namespace commons {

/* if size == 64, then size = 64, no change
 * if size = 63, then size = 64
 * if size = 33, then size = 40, always upgraded to the higher value
 *
 */
constexpr uint16_t alignToNextHighestValueDivisableBy8(uint16_t size) {
  return static_cast<uint16_t>((size % 8 == 0) ? size : ((size & (~7)) + 8));
}

/**
 * Sparse Partial Keys are used to store the discriminative bits required to distinguish entries in a linearized binary patricia trie.
 *
 * For a single binary patricia trie the set of discriminative bits consists of all discriminative bit positions used in its BiNodes.
 * Partial Keys therefore consists only of those bits of a the stored key which correspon to a discriminative bit.
 * Sparse partial keys are an optimization of partial keys and can be used to construct linearized representations of binary patricia tries.
 * For each entry only those bits in the sparse partial keys are set, which correspond to a BiNodes along the path from the binary patricia trie root.
 * All other bits are set to 0 and are therefore intentially left undefined.
 *
 * To clarify the notion of sparse partial keys we illustrate the conversion of a binary patricia trie consisting of 7 entries into its linearized representation.
 * This is an ASCII art version of Figure 5 of "HOT: A Height Optimized Trie Index"
 *                                                                                           3 4 6 8 9            3 4 6 8 9
 *             (bit 3, 3-bit prefix 011)           |Values |   Raw Key   |Bit Positions|Partial key (dense) |Partial key (sparse)|
 *                    /  \                         |=======|=============|=============|====================|====================|
 *                  /     \                        |  v1   |  0110100101 |  {3,6,8,}   |     0 1 0 0 1      |     0 0 0 0 0      |
 *              010/       \1                      |-------|-------------|-------------|--------------------|--------------------|
 *               /          \                      |  v2   |  0110100110 |  {3,6,8}    |     0 1 0 1 0      |     0 0 0 1 0      |
 *              /            \                     |-------|-------------|-------------|--------------------|--------------------|
 *        (bit 6))          (bit 4)                |  v3   |  0110101010 |  {3,6,9}    |     0 1 1 1 0      |     0 0 1 0 0      |
 *         /     \            /  \                 |-------|-------------|-------------|--------------------|--------------------|
 *      01/    101\    010110/    \ 1010           |  v4   |  0110101011 |  {3,6,9}    |     0 1 1 1 1      |     0 0 1 0 1      |
 *       /        \         /      \               |-------|-------------|-------------|--------------------|--------------------|
 *    (bit 8)  (bit 9)    v5    (bit 8)            |  v5   |  0111010110 |   {3,4}     |     1 0 0 1 0      |     1 0 0 0 0      |
 *      / \      / \              / \              |-------|-------------|-------------|--------------------|--------------------|
 *   01/   \10 0/  \1        01  /   \ 11          |  v6   |  0111101001 |  {3,4,8}    |     1 1 1 0 1      |     1 1 0 0 0      |
 *    /     \  /    \           /     \            |-------|-------------|-------------|--------------------|--------------------|
 *   v1     v2 v3   v4         v6      v7          |  v7   |  0111101011 |  {3,4,8}    |     1 1 1 1 1      |     1 1 0 1 0      |
 *                                                 |=====================|=============|====================|====================|
 */
template<typename PartialKeyType>
struct alignas(8) SparsePartialKeys {
  void *operator new(size_t /* baseSize */, uint16_t const numberEntries);

  void operator delete(void *);

  PartialKeyType mEntries[1];

  /**
   * Search returns a mask corresponding to all partial keys complying to the dense partial search key.
   * The bit positions in the result mask are indexed from the least to the most singificant bit
   *
   * Compliance for a sparse partial key is defined in the following:
   *
   * (densePartialKey & sparsePartialKey) === sparsePartialKey
   *
   *
   * @param densePartialSearchKey the dense partial key of the search key to search matching entries for
   * @return the resulting mask with each bit representing the result of a single compressed mask. bit 0 (least significant) correspond to the mask 0, bit 1 corresponds to mask 1 and so forth.
   */
  inline uint32_t search(PartialKeyType const densePartialSearchKey) const;

  inline uint32_t search(PartialKeyType const densePartialSearchKey, uint64_t mask) const;

  /**
   * Determines all dense partial keys which match a given partial key pattern.
   *
   * This method exectues for each sparse partial key the following operation (sparsePartialKey[i]): sparsePartialKey[i] & partialKeyPattern == partialKeyPattern
   *
   * @param partialKeyPattern the pattern to use for searchin compressed mask
   * @return the resulting mask with each bit representing the result of a single compressed mask. bit 0 (least significant) correspond to the mask 0, bit 1 corresponds to mask 1 and so forth.
   */
  inline uint32_t findMasksByPattern(PartialKeyType const partialKeyPattern) const {
    #ifdef PING_DEBUG3
    std::cout << "findMasksByPattern function: partialKeyPattern----begins-------- " << std::endl;
#endif
    __m256i searchRegister = broadcastToSIMDRegister(partialKeyPattern);
    #ifdef PING_DEBUG3
    uint8_t *val1 = (uint8_t*) &searchRegister;
    std::cout << "-------searchRegister: broadcastToSIMDRegister with partialKeyPattern-------" << std::endl;
    printf("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
        val1[6], val1[7], val1[8], val1[9], val1[10], val1[11], val1[12], val1[13], val1[14], val1[15],
        val1[16], val1[17], val1[18], val1[19], val1[20], val1[21], val1[22], val1[23], val1[24], val1[25],
        val1[26], val1[27], val1[28], val1[29], val1[30], val1[31]);
    #endif
    //std::cout << "findMasksByPattern = " << std::bitset<32>(findMasksByPattern(searchRegister, searchRegister)) << std::endl;

    return findMasksByPattern(searchRegister, searchRegister);
  }

  private:
  inline uint32_t findMasksByPattern(__m256i const usedBitsMask, __m256i const expectedBitsMask) const;

  inline __m256i broadcastToSIMDRegister(PartialKeyType const mask) const;

  public:

  /**
   * This method determines all entries which are contained in a common subtree.
   * The subtree is defined, by all prefix bits used and the expected prefix bits value
   *
   * @param usedPrefixBitsPattern a partial key pattern, which has all bits set, which correspond to parent BiNodes of the  regarding subtree
   * @param expectedPrefixBits  a partial key pattern, which has all bits set, according to the paths taken from the root node to the parent BiNode of the regarding subtree
   * @return a resulting bit mask with each bit represent whether the corresponding entry is part of the requested subtree or not.
   */
  inline uint32_t getAffectedSubtreeMask(PartialKeyType usedPrefixBitsPattern, PartialKeyType const expectedPrefixBits) const {
    #ifdef PING_DEBUG3
    printf("getAffectedSubtreeMask function () ----begins\n");
#endif
    //assumed this should be the same as
    //affectedMaskBitsRegister = broadcastToSIMDRegister(affectedBitsMask)
    //expectedMaskBitsRegister = broadcastToSIMDRegister(expectedMaskBits)
    //return findMasksByPattern(affectedMaskBitsRegister, expectedMaskBitsRegister) & usedEntriesMask;

    __m256i prefixBITSSIMDMask = broadcastToSIMDRegister(usedPrefixBitsPattern);
    __m256i prefixMask = broadcastToSIMDRegister(expectedPrefixBits);

    unsigned int affectedSubtreeMask = findMasksByPattern(prefixBITSSIMDMask, prefixMask);

    //uint affectedSubtreeMask = findMasksByPattern(mEntries[entryIndex] & subtreePrefixMask) & usedEntriesMask;
    //at least the zero mask must match
    assert(affectedSubtreeMask != 0);
    #ifdef PING_DEBUG3
    std::cout << "affectedSubtreeMask = searchResult after move_mask = " << std::bitset<32>(affectedSubtreeMask) << std::endl;
    printf("======getAffectedSubtreeMask function () ----end--\n");
    #endif
    return affectedSubtreeMask;
  }

  /**
   *
   * in the case of the following tree structure:
   *               d
   *            /    \
   *           b      f
   *          / \    / \
   *         a  c   e  g
   * Index   0  1   2  3
   *
   * If the provided index is 2 corresponding the smallest common subtree containing e consists of the nodes { e, f, g }
   * and therefore the discriminative bit value for this entry is 0 (in the left side of the subtree).
   *
   * If the provided index is 1 corresponding to entry c, the smallest common subtree containing c consists of the nodes { a, b, c }
   * and therefore the discriminative bit value of this entries 1 (in the right side of the subtree).
   *
   * in the case of the following tree structure
   *                    f
   *                 /    \
   *                d      g
   *              /   \
   *             b     e
   *            / \
   *           a   c
   * Index     0   1   2   3
   *
   * If the provided index is 2 correspondig to entry e, the smallest common subtree containing e consists of the nodes { a, b, c, d, e }
   * As e is on the right side of this subtree, the discriminative bit's value in this case is 1
   *
   *
   * @param indexOfEntry The index of the entry to obtain the discriminative bits value for
   * @return The discriminative bit value of the discriminative bit discriminating an entry from its direct neighbour (regardless if the direct neighbour is an inner or a leaf node).
   * if the bit is in the left side of the subtree: return 0
   * otherwise, return 1 (in the right side of the subtree)
   */
  inline bool determineValueOfDiscriminatingBit(size_t indexOfEntry, size_t mNumberEntries) const {
    #ifdef PING_DEBUG3
    printf("---determineValueOfDiscriminatingBit function () starts--return 0-left, 1-right--\n");
    #endif
    bool discriminativeBitValue;

    if(indexOfEntry == 0) {
      discriminativeBitValue = false;
    } else if(indexOfEntry == (mNumberEntries - 1)) {
      discriminativeBitValue = true;
    } else {
      #ifdef PING_DEBUG3
      printf("mEntries[indexOfEntry - 1] = %d, mEntries[indexOfEntry] = %d, mEntries[indexOfEntry + 1] = %d\n", mEntries[indexOfEntry - 1], mEntries[indexOfEntry], mEntries[indexOfEntry + 1]);
      #endif
      //Be aware that the masks are not order preserving, as the bits may not be in the correct order little vs. big endian and several included bytes
      discriminativeBitValue = (mEntries[indexOfEntry - 1]&mEntries[indexOfEntry]) >= (mEntries[indexOfEntry]&mEntries[indexOfEntry + 1]);
    }
    #ifdef PING_DEBUG3
    printf("---determineValueOfDiscriminatingBit function () ends--return discriminativeBitValue = %d--\n", discriminativeBitValue);
    #endif
    return discriminativeBitValue;
  }

  /**
   * Get Relevant bits detects the key bits used for discriminating new entries in the given range.
   * These bits are determined by comparing successing masks in this range.
   * Whenever a mask has a bit set which is not set in its predecessor these bit is added to the set of relevant bits.
   * The reason is that if masks are stored in an orderpreserving way for a mask to be large than its predecessor it has to set
   * exactly one more bit.
   * By using this algorithm the bits of the first mask occuring in the range of masks are always ignored.
   *
   * @param firstIndexInRange the first index of the range of entries to determine the relevant bits for
   * @param numberEntriesInRange the number entries in the range of entries to use for determining the relevant bits
   * @return a mask with only the relevant bits set.
   */
  inline PartialKeyType getRelevantBitsForRange(uint32_t const firstIndexInRange, uint32_t const numberEntriesInRange) const {
    #ifdef PING_DEBUG3
    printf("--getRelevantBitsForRange function () starts---\n");
    #endif
    PartialKeyType relevantBits = 0;
    uint32_t firstIndexOutOfRange = firstIndexInRange + numberEntriesInRange;
    #ifdef PING_DEBUG3
    printf("firstIndexInRange = %d, firstIndexOutOfRange = %d\n", firstIndexInRange, firstIndexOutOfRange);
    #endif
    for(uint32_t i = firstIndexInRange + 1; i < firstIndexOutOfRange; ++i) {
      #ifdef PING_DEBUG3
      printf("i = %d, mEntries[i] = %x, mEntries[i - 1] = %x\n", i, mEntries[i], mEntries[i - 1]);
      #endif
      relevantBits |= (mEntries[i] & ~mEntries[i - 1]);
    }
    #ifdef PING_DEBUG
    printf("--getRelevantBitsForRange function () ends--return relevantBits = %p\n", relevantBits);
    #endif
    return relevantBits;
  }

  /**
   * Gets a partial key which has all discriminative bits set, which are required to distinguish all but the entry, which is intended to be removed.
   *
   * @param numberEntries the total number of entries
   * @param indexOfEntryToIgnore  the index of the entry, which shall be removed
   * @return partial key which has all neccessary discriminative bits set
   */
  inline PartialKeyType getRelevantBitsForAllExceptOneEntry(uint32_t const numberEntries, uint32_t indexOfEntryToIgnore) const {
    #ifdef PING_DEBUG3
    printf("---getRelevantBitsForAllExceptOneEntry function () starts----\n");
#endif
    size_t numberEntriesInFirstRange = indexOfEntryToIgnore + static_cast<size_t>(!determineValueOfDiscriminatingBit(indexOfEntryToIgnore, numberEntries));
#ifdef PING_DEBUG3
    printf("numberEntriesInFirstRange = %zu\n", numberEntriesInFirstRange);
    #endif
    PartialKeyType relevantBitsInFirstPart = getRelevantBitsForRange(0, numberEntriesInFirstRange);
    PartialKeyType relevantBitsInSecondPart = getRelevantBitsForRange(numberEntriesInFirstRange, numberEntries - numberEntriesInFirstRange);
    #ifdef PING_DEBUG3
    std::cout << "getRelevantBitsForAllExceptOneEntry = " << std::bitset<8>(relevantBitsInFirstPart | relevantBitsInSecondPart) << std::endl;
    #endif
    return relevantBitsInFirstPart | relevantBitsInSecondPart;
  }

  static inline uint16_t estimateSize(uint16_t numberEntries) {
    #ifdef PING_DEBUG
    std::cout << "PartialKeyType size = " << sizeof(PartialKeyType) << std::endl;
    #endif
    return alignToNextHighestValueDivisableBy8(numberEntries * sizeof(PartialKeyType));
  }

  /**
   * @param maskBitMapping maps from the absoluteBitPosition to its maskPosition
   */
  inline void printMasks(uint32_t maskOfEntriesToPrint, std::map<uint16_t, uint16_t> const & maskBitMapping, std::ostream & outputStream = std::cout) const {
    while(maskOfEntriesToPrint > 0) {
      uint entryIndex = __tzcnt_u32(maskOfEntriesToPrint);
      std::bitset<sizeof(PartialKeyType) * 8> maskBits(mEntries[entryIndex]);
      outputStream << "mask[" << entryIndex << "] = \toriginal: " << maskBits << "\tmapped: ";
      printMaskWithMapping(mEntries[entryIndex], maskBitMapping, outputStream);
      outputStream << std::endl;
      maskOfEntriesToPrint &= (~(1u << entryIndex));
    }
  }

  static inline void printMaskWithMapping(PartialKeyType mask, std::map<uint16_t, uint16_t> const & maskBitMapping, std::ostream & outputStream) {
    std::bitset<convertBytesToBits(sizeof(PartialKeyType))> maskBits(mask);
    for(auto mapEntry : maskBitMapping) {
      uint64_t maskBitIndex = mapEntry.second;
      outputStream << maskBits[maskBitIndex];
    }
  }


  inline void printMasks(uint32_t maskOfEntriesToPrint, std::ostream & outputStream = std::cout) const {
    while(maskOfEntriesToPrint > 0) {
      uint entryIndex = __tzcnt_u32(maskOfEntriesToPrint);

      std::bitset<sizeof(PartialKeyType) * 8> maskBits(mEntries[entryIndex]);
      outputStream << "mask[" << entryIndex << "] = " << maskBits << std::endl;

      maskOfEntriesToPrint &= (~(1u << entryIndex));
    }
  }

  private:
  // Prevent heap allocation
  void * operator new   (size_t) = delete;
  void * operator new[] (size_t) = delete;
  void operator delete[] (void*) = delete;
};

      template<typename PartialKeyType> void* SparsePartialKeys<PartialKeyType>::operator new (size_t /* baseSize */, uint16_t const numberEntries) {
    assert(numberEntries >= 2);

    constexpr size_t paddingElements = (32 - 8)/sizeof(PartialKeyType);
    size_t estimatedNumberElements = estimateSize(numberEntries)/sizeof(PartialKeyType);


    return new PartialKeyType[estimatedNumberElements + paddingElements];
  };
  template<typename PartialKeyType> void SparsePartialKeys<PartialKeyType>::operator delete (void * rawMemory) {
    PartialKeyType* masks = reinterpret_cast<PartialKeyType*>(rawMemory);
    delete [] masks;
  }

//  template<>
//  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint8_t>::search(uint8_t const uncompressedSearchMask) const {
//    #ifdef PING_DEBUG3
//    std::cout << "Search function: SparsePartialKeys<uint8_t>" << std::endl;
//    std::cout << "uncompressedSearchMask = " << std::bitset<8>(uncompressedSearchMask) << std::endl;
//#endif
//    __m256i searchRegister = _mm256_set1_epi8(uncompressedSearchMask); //2 instr
//    #ifdef PING_DEBUG3
//    uint8_t *val1 = (uint8_t*) &searchRegister;
//    printf("searchRegister = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
//        val1[6], val1[7], val1[8], val1[9], val1[10], val1[11], val1[12], val1[13], val1[14], val1[15],
//        val1[16], val1[17], val1[18], val1[19], val1[20], val1[21], val1[22], val1[23], val1[24], val1[25],
//        val1[26], val1[27], val1[28], val1[29], val1[30], val1[31]);
//#endif
//    __m256i haystack = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr
//    #ifdef PING_DEBUG3
//    uint8_t *val = (uint8_t*) &haystack;
//    std::cout << "load mEntries _m256i register: ";
//    printf("haystack = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val[0], val[1], val[2], val[3], val[4], val[5],
//        val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15],
//        val[16], val[17], val[18], val[19], val[20], val[21], val[22], val[23], val[24], val[25],
//        val[26], val[27], val[28], val[29], val[30], val[31]);
//    #endif
//#ifdef USE_AVX512
//    uint32_t const resultMask = _mm256_cmpeq_epi8_mask (_mm256_and_si256(haystack, searchRegister), haystack);
//#else
//    __m256i searchResult = _mm256_cmpeq_epi8(_mm256_and_si256(haystack, searchRegister), haystack);
//    #ifdef PING_DEBUG3
//    uint8_t *val2 = (uint8_t*) &searchResult;
//    printf("searchResult = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val2[0], val2[1], val2[2], val2[3], val2[4], val2[5],
//        val2[6], val2[7], val2[8], val2[9], val2[10], val2[11], val2[12], val2[13], val2[14], val2[15],
//        val2[16], val2[17], val2[18], val2[19], val2[20], val2[21], val2[22], val2[23], val2[24], val2[25],
//        val2[26], val2[27], val2[28], val2[29], val2[30], val2[31]);
//    #endif
//    uint32_t const resultMask = static_cast<uint32_t>(_mm256_movemask_epi8(searchResult));
//#endif
//    #ifdef PING_DEBUG3
//    std::cout << "resultMask = " << std::bitset<32>(resultMask) << std::endl;
//    #endif
//    return resultMask;
//  }


  /*
   *
   * New uint32_t SparsePartialKeys<uint8_t>::search, added shr_count argument
   * 09/13/2020, change shr_count to extract_mask variable
   * 11/10/2020, added the uint32_t wildcard_affected_entry_mask argument
   * get the better affectedSubtreeInformation
   * searchEntry & entry = A, compare A with searchentry get B
   * use B & currentRst = wildcard_affected_entry_mask
   */

  template<>
  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint8_t>::search(uint8_t const uncompressedSearchMask) const {
    #ifdef PING_DEBUG3
    std::cout << "New Search function: SparsePartialKeys<uint8_t>---added count argument" << std::endl;
    std::cout << "uncompressedSearchMask = " << std::bitset<8>(uncompressedSearchMask) << std::endl;
    std::cout << "extract_mask = " << std::bitset<64>(extract_mask) << std::endl;

#endif
    __m256i searchRegister = _mm256_set1_epi8(uncompressedSearchMask); //2 instr

//#ifdef PING_DEBUG3
//    uint8_t *val1 = (uint8_t*) &searchRegister;
//    printf("searchRegister = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
//        val1[6], val1[7], val1[8], val1[9], val1[10], val1[11], val1[12], val1[13], val1[14], val1[15],
//        val1[16], val1[17], val1[18], val1[19], val1[20], val1[21], val1[22], val1[23], val1[24], val1[25],
//        val1[26], val1[27], val1[28], val1[29], val1[30], val1[31]);
//#endif

    __m256i haystack = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr

    uint8_t *val = (uint8_t*) &haystack;

//#ifdef PING_DEBUG3

//    std::cout << "load mEntries _m256i register: ";
//    printf("haystack = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val[0], val[1], val[2], val[3], val[4], val[5],
//        val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15],
//        val[16], val[17], val[18], val[19], val[20], val[21], val[22], val[23], val[24], val[25],
//        val[26], val[27], val[28], val[29], val[30], val[31]);
//    #endif

//    shr_count = 0;  //  hard coded shr_count = 0, in order to avoid the right shift for debugging

if (extract_mask == 0) {

    __m256i searchResult = _mm256_cmpeq_epi8(_mm256_and_si256(haystack, searchRegister), haystack);

    uint32_t const resultMask = static_cast<uint32_t>(_mm256_movemask_epi8(searchResult));

    #ifdef PING_DEBUG3
    std::cout << "resultMask = " << std::bitset<32>(resultMask) << std::endl;
    #endif
    return resultMask;

} else {
  /*
   * each item in mEntries (haystack) need to do bits extractions
   * using _pext_32() to change haystack
   */
__m256i haystack_1;
uint8_t *val_1 = (uint8_t*) &haystack_1;
for (int i = 0; i < 32; ++i) {
  val_1[i] = static_cast<uint8_t> (_pext_u64(val[i], extract_mask));
}

#ifdef PING_DEBUG3
printf("haystack_1       = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val_1[0], val_1[1], val_1[2], val_1[3], val_1[4], val_1[5],
    val_1[6], val_1[7], val_1[8], val_1[9], val_1[10], val_1[11], val_1[12], val_1[13], val_1[14], val_1[15],
    val_1[16], val_1[17], val_1[18], val_1[19], val_1[20], val_1[21], val_1[22], val_1[23], val_1[24], val_1[25],
    val_1[26], val_1[27], val_1[28], val_1[29], val_1[30], val_1[31]);
#endif



    __m256i searchResult = _mm256_cmpeq_epi8(_mm256_and_si256(haystack_1, searchRegister), haystack_1);
    #ifdef PING_DEBUG3
    uint8_t *val2_2 = (uint8_t*) &searchResult;
    printf("searchResult = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val2_2[0], val2_2[1], val2_2[2], val2_2[3], val2_2[4], val2_2[5],
        val2_2[6], val2_2[7], val2_2[8], val2_2[9], val2_2[10], val2_2[11], val2_2[12], val2_2[13], val2_2[14], val2_2[15],
        val2_2[16], val2_2[17], val2_2[18], val2_2[19], val2_2[20], val2_2[21], val2_2[22], val2_2[23], val2_2[24], val2_2[25],
        val2_2[26], val2_2[27], val2_2[28], val2_2[29], val2_2[30], val2_2[31]);
    #endif

    __m256i searchResult1 = _mm256_cmpeq_epi8(_mm256_and_si256(haystack_1, searchRegister), searchRegister);

    wildcard_affected_entry_mask = static_cast<uint32_t>(_mm256_movemask_epi8(searchResult & searchResult1));



#ifdef PING_DEBUG3
std::cout << "wildcard_affected_entry_mask = " << std::bitset<32>(wildcard_affected_entry_mask) << std::endl;
#endif

    uint32_t const resultMask = static_cast<uint32_t>(_mm256_movemask_epi8(searchResult));

    #ifdef PING_DEBUG3
    std::cout << "resultMask = " << std::bitset<32>(resultMask) << std::endl;
    #endif
    return resultMask;




}



  }


//  template<>
//  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint16_t>::search(uint16_t const uncompressedSearchMask) const {
//    #ifdef PING_DEBUG3
//    std::cout << "SparsePartialKeys<uint16_t>" << std::endl;
//    #endif
//#ifdef USE_AVX512
//    __m512i searchRegister = _mm512_set1_epi16(uncompressedSearchMask); //2 instr
//    __m512i haystack = _mm512_loadu_si512(reinterpret_cast<__m512i const *>(mEntries)); //3 instr
//    return static_cast<uint32_t>(_mm512_cmpeq_epi16_mask(_mm512_and_si512(haystack, searchRegister), haystack));
//#else
//    __m256i searchRegister = _mm256_set1_epi16(uncompressedSearchMask); //2 instr

//    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr
//    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16)); //4 instr

//    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

//    __m256i searchResult1 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack1, searchRegister), haystack1);
//    __m256i searchResult2 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack2, searchRegister), haystack2);

//    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
//                                                                                 searchResult1, searchResult2
//                                                                                 ), perm_mask);
//#ifdef PING_DEBUG3
//    std::cout << "Search result = " << static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult)) << std::endl;
//#endif
//    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
//#endif
//  }

//  template<>
//  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint32_t>::search(uint32_t const uncompressedSearchMask) const {
//    #ifdef PING_DEBUG3
//    std::cout << "SparsePartialKeys<uint32_t>" << std::endl;
//    #endif
//    __m256i searchRegister = _mm256_set1_epi32(uncompressedSearchMask); //2 instr

//    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries));
//    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 8));
//    __m256i haystack3 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16));
//    __m256i haystack4 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 24)); //27 instr

//    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

//    __m256i searchResult1 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1, searchRegister), haystack1);
//    __m256i searchResult2 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2, searchRegister), haystack2);
//    __m256i searchResult3 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3, searchRegister), haystack3);
//    __m256i searchResult4 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4, searchRegister), haystack4); //35 + 8 = 43

//    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
//                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult1, searchResult2), perm_mask),
//                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult3, searchResult4), perm_mask)
//                                                                                 ), perm_mask);

//    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
//  }

  template<>
  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint16_t>::search(uint16_t const uncompressedSearchMask) const {
    #ifdef PING_DEBUG3
    std::cout << "------New Search function: SparsePartialKeys<uint16_t>---added count argument------" << std::endl;
    std::cout << "uncompressedSearchMask = " << std::bitset<16>(uncompressedSearchMask) << std::endl;

    #endif

    __m256i searchRegister = _mm256_set1_epi16(uncompressedSearchMask); //2 instr

#ifdef PING_DEBUG3
    uint16_t *val1 = (uint16_t*) &searchRegister;
    printf("searchRegister = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
        val1[6], val1[7], val1[8], val1[9], val1[10], val1[11], val1[12], val1[13], val1[14], val1[15]);
#endif


    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr
    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16)); //4 instr

uint16_t *val2 = (uint16_t*) &haystack1;
#ifdef PING_DEBUG3

    printf("haystack1 = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val2[0], val2[1], val2[2], val2[3], val2[4], val2[5],
        val2[6], val2[7], val2[8], val2[9], val2[10], val2[11], val2[12], val2[13], val2[14], val2[15]);
#endif
uint16_t *val3 = (uint16_t*) &haystack2;
#ifdef PING_DEBUG3

    printf("haystack2 = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val3[0], val3[1], val3[2], val3[3], val3[4], val3[5],
        val3[6], val3[7], val3[8], val3[9], val3[10], val3[11], val3[12], val3[13], val3[14], val3[15]);
#endif

    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

    if (extract_mask == 0) {
      __m256i searchResult1 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack1, searchRegister), haystack1);
      __m256i searchResult2 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack2, searchRegister), haystack2);

  #ifdef PING_DEBUG3
      uint16_t *val4 = (uint16_t*) &searchResult1;
      printf("searchResult1 = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val4[0], val4[1], val4[2], val4[3], val4[4], val4[5],
          val4[6], val4[7], val4[8], val4[9], val4[10], val4[11], val4[12], val4[13], val4[14], val4[15]);
  #endif

  #ifdef PING_DEBUG3
      uint16_t *val5 = (uint16_t*) &searchResult2;
      printf("searchResult2 = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val5[0], val5[1], val5[2], val5[3], val5[4], val5[5],
          val5[6], val5[7], val5[8], val5[9], val5[10], val5[11], val5[12], val5[13], val5[14], val5[15]);
  #endif

//      __m256i middle = _mm256_packs_epi16( searchResult1, searchResult2);




      __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                   searchResult1, searchResult2

                                                                                   ), perm_mask);

  #ifdef PING_DEBUG3
      uint8_t *val7 = (uint8_t*) &intermediateResult;
      printf("intermediateResult = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val7[0], val7[1], val7[2], val7[3], val7[4], val7[5],
          val7[6], val7[7], val7[8], val7[9], val7[10], val7[11], val7[12], val7[13], val7[14], val7[15],
          val7[16], val7[17], val7[18], val7[19], val7[20], val7[21], val7[22], val7[23], val7[24], val7[25],
          val7[26], val7[27], val7[28], val7[29], val7[30], val7[31]);
  #endif

  #ifdef PING_DEBUG3
      std::cout << "Search result = " << std::bitset<32>(static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult))) << std::endl;
  #endif
      return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));

    } else {


      __m256i haystack1_1;
      uint16_t *val_1 = (uint16_t*) &haystack1_1;
      for (int i = 0; i < 16; ++i) {
        val_1[i] = static_cast<uint16_t> (_pext_u64(val2[i], extract_mask));
      }


      __m256i haystack2_1;
      uint16_t *val_2 = (uint16_t*) &haystack2_1;
      for (int i = 0; i < 16; ++i) {
        val_2[i] = static_cast<uint16_t> (_pext_u64(val3[i], extract_mask));
      }

      __m256i searchResult1 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack1_1, searchRegister), haystack1_1);
      __m256i searchResult2 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack2_1, searchRegister), haystack2_1);

      __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                   searchResult1, searchResult2

                                                                                   ), perm_mask);

      uint32_t A = static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));



      __m256i searchResult11 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack1_1, searchRegister), searchRegister);
      __m256i searchResult22 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack2_1, searchRegister), searchRegister);

      __m256i intermediateResult1 = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                   searchResult11, searchResult22

                                                                                   ), perm_mask);

      uint32_t B = static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult1));

      wildcard_affected_entry_mask = A & B;



  #ifdef PING_DEBUG3
  std::cout << "------uint16_t---------wildcard_affected_entry_mask = " << std::bitset<32>(wildcard_affected_entry_mask) << std::endl;
  #endif


#ifdef PING_DEBUG3
    std::cout << "Search result = " << std::bitset<32>(static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult))) << std::endl;
#endif
    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));

    }



  }

//  template<>
//  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint32_t>::search(uint32_t const uncompressedSearchMask) const {
//    #ifdef PING_DEBUG3
//    std::cout << "---New------SparsePartialKeys<uint32_t>::findMasksByPattern-------function starts-" << std::endl;
//    std::cout << "uncompressedSearchMask = " << std::bitset<32>(uncompressedSearchMask) << std::endl;
//    std::cout << "shr_count = " << shr_count << std::endl;
//    #endif
//    __m256i searchRegister = _mm256_set1_epi32(uncompressedSearchMask); //2 instr

//#ifdef PING_DEBUG3
//    uint16_t *val1 = (uint32_t*) &searchRegister;
//    printf("searchRegister = %i %i %i %i %i %i %i %i\n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
//        val1[6], val1[7]);
//#endif


//    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries));
//    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 8));
//    __m256i haystack3 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16));
//    __m256i haystack4 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 24)); //27 instr

//#ifdef PING_DEBUG3
//    uint32_t *val2 = (uint32_t*) &haystack1;
//    printf("haystack1 = %i %i %i %i %i %i %i %i\n", val2[0], val2[1], val2[2], val2[3], val2[4], val2[5],
//        val2[6], val2[7]);
//#endif

//#ifdef PING_DEBUG3
//    uint32_t *val3 = (uint32_t*) &haystack2;
//    printf("haystack2 = %i %i %i %i %i %i %i %i\n", val3[0], val3[1], val3[2], val3[3], val3[4], val3[5],
//        val3[6], val3[7]);
//#endif

//#ifdef PING_DEBUG3
//    uint32_t *val4 = (uint32_t*) &haystack3;
//    printf("haystack3 = %i %i %i %i %i %i %i %i\n", val4[0], val4[1], val4[2], val4[3], val4[4], val4[5],
//        val4[6], val4[7]);
//#endif

//#ifdef PING_DEBUG3
//    uint32_t *val5 = (uint32_t*) &haystack4;
//    printf("haystack4 = %i %i %i %i %i %i %i %i\n", val5[0], val5[1], val5[2], val5[3], val5[4], val5[5],
//        val5[6], val5[7]);
//#endif


//    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

//    __m256i searchResult1 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1, searchRegister), haystack1);
//    __m256i searchResult2 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2, searchRegister), haystack2);
//    __m256i searchResult3 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3, searchRegister), haystack3);
//    __m256i searchResult4 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4, searchRegister), haystack4); //35 + 8 = 43

//    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
//                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult1, searchResult2), perm_mask),
//                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult3, searchResult4), perm_mask)
//                                                                                 ), perm_mask);

//#ifdef PING_DEBUG3
//    std::cout << "Search result = " << std::bitset<32>(static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult))) << std::endl;
//#endif

//    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
//  }


  template<>
  inline __attribute__((always_inline)) uint32_t SparsePartialKeys<uint32_t>::search(uint32_t const uncompressedSearchMask) const {
    #ifdef PING_DEBUG3
    std::cout << "---New------SparsePartialKeys<uint32_t>::search------function starts-" << std::endl;
    std::cout << "uncompressedSearchMask = " << std::bitset<32>(uncompressedSearchMask) << std::endl;
    std::cout << "extract_mask = " << std::bitset<64>(extract_mask) << std::endl;
    #endif
    __m256i searchRegister = _mm256_set1_epi32(uncompressedSearchMask); //2 instr

#ifdef PING_DEBUG3
    uint32_t *val1 = (uint32_t*) &searchRegister;
    printf("searchRegister = %i %i %i %i %i %i %i %i\n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
        val1[6], val1[7]);
#endif


    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries));
    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 8));
    __m256i haystack3 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16));
    __m256i haystack4 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 24)); //27 instr

    uint32_t *val2 = (uint32_t*) &haystack1;
#ifdef PING_DEBUG3

    printf("haystack1 = %u %u %u %u %u %u %u %u\n", val2[0], val2[1], val2[2], val2[3], val2[4], val2[5],
        val2[6], val2[7]);
#endif
uint32_t *val3 = (uint32_t*) &haystack2;
#ifdef PING_DEBUG3

    printf("haystack2 = %u %u %u %u %u %u %u %u\n", val3[0], val3[1], val3[2], val3[3], val3[4], val3[5],
        val3[6], val3[7]);
#endif
uint32_t *val4 = (uint32_t*) &haystack3;
#ifdef PING_DEBUG3

    printf("haystack3 = %u %u %u %u %u %u %u %u\n", val4[0], val4[1], val4[2], val4[3], val4[4], val4[5],
        val4[6], val4[7]);
#endif

    uint32_t *val5 = (uint32_t*) &haystack4;
#ifdef PING_DEBUG3

    printf("haystack4 = %u %u %u %u %u %u %u %u\n", val5[0], val5[1], val5[2], val5[3], val5[4], val5[5],
        val5[6], val5[7]);
#endif


    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

    if (extract_mask == 0) {

    __m256i searchResult1 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1, searchRegister), haystack1);
    __m256i searchResult2 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2, searchRegister), haystack2);
    __m256i searchResult3 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3, searchRegister), haystack3);
    __m256i searchResult4 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4, searchRegister), haystack4); //35 + 8 = 43

    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult1, searchResult2), perm_mask),
                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult3, searchResult4), perm_mask)
                                                                                 ), perm_mask);

#ifdef PING_DEBUG3
    std::cout << "Search result = " << std::bitset<32>(static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult))) << std::endl;
#endif

    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
    } else {
      //  need to do some right shift for the haystack, try to find the correct entryIndex

      __m256i haystack1_1;
      uint32_t *val_1 = (uint32_t*) &haystack1_1;
      for (int i = 0; i < 8; ++i) {
        val_1[i] = static_cast<uint32_t> (_pext_u64(val2[i], extract_mask));
      }

      __m256i haystack2_1;
      uint32_t *val_2 = (uint32_t*) &haystack2_1;
      for (int i = 0; i < 8; ++i) {
        val_2[i] = static_cast<uint32_t> (_pext_u64(val3[i], extract_mask));
      }

      __m256i haystack3_1;
      uint32_t *val_3 = (uint32_t*) &haystack3_1;
      for (int i = 0; i < 8; ++i) {
        val_3[i] = static_cast<uint32_t> (_pext_u64(val4[i], extract_mask));
      }

      __m256i haystack4_1;
      uint32_t *val_4 = (uint32_t*) &haystack4_1;
      for (int i = 0; i < 8; ++i) {
        val_4[i] = static_cast<uint32_t> (_pext_u64(val5[i], extract_mask));
      }

      __m256i searchResult1 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1_1, searchRegister), haystack1_1);
      __m256i searchResult2 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2_1, searchRegister), haystack2_1);
      __m256i searchResult3 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3_1, searchRegister), haystack3_1);
      __m256i searchResult4 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4_1, searchRegister), haystack4_1); //35 + 8 = 43

      __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                   _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult1, searchResult2), perm_mask),
                                                                                   _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult3, searchResult4), perm_mask)
                                                                                   ), perm_mask);

      uint32_t A = static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));


      __m256i searchResult11 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1_1, searchRegister), searchRegister);
      __m256i searchResult22 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2_1, searchRegister), searchRegister);
      __m256i searchResult33 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3_1, searchRegister), searchRegister);
      __m256i searchResult44 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4_1, searchRegister), searchRegister); //35 + 8 = 43

      __m256i intermediateResult1 = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                   _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult11, searchResult22), perm_mask),
                                                                                   _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult33, searchResult44), perm_mask)
                                                                                   ), perm_mask);


      uint32_t B = static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult1));

      wildcard_affected_entry_mask = A & B;


#ifdef PING_DEBUG3
std::cout << "------uint32_t---------wildcard_affected_entry_mask = " << std::bitset<32>(wildcard_affected_entry_mask) << std::endl;
#endif



  #ifdef PING_DEBUG3
      std::cout << "Search result = " << std::bitset<32>(static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult))) << std::endl;
  #endif

      return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));



    }
  }




  template<>
  inline uint32_t SparsePartialKeys<uint8_t>::findMasksByPattern(__m256i consideredBitsRegister, __m256i expectedBitsRegister) const {
    #ifdef PING_DEBUG3
    std::cout << "--SparsePartialKeys<uint8_t>::findMasksByPattern---two _m256i register--function begin:" << std::endl;
    uint8_t *val = (uint8_t*) &consideredBitsRegister;
    std::cout << "----consideredBitsRegister-----:" << std::endl;
    printf("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val[0], val[1], val[2], val[3], val[4], val[5],
        val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15],
        val[16], val[17], val[18], val[19], val[20], val[21], val[22], val[23], val[24], val[25],
        val[26], val[27], val[28], val[29], val[30], val[31]);
    std::cout << "----expectedBitsRegister-----:" << std::endl;
    uint8_t *val2 = (uint8_t*) &expectedBitsRegister;
    printf("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val2[0], val2[1], val2[2], val2[3], val2[4], val2[5],
        val2[6], val2[7], val2[8], val2[9], val2[10], val2[11], val2[12], val2[13], val2[14], val2[15],
        val2[16], val2[17], val2[18], val2[19], val2[20], val2[21], val2[22], val2[23], val2[24], val2[25],
        val2[26], val2[27], val2[28], val2[29], val2[30], val2[31]);
#endif
    __m256i haystack = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr
    #ifdef PING_DEBUG3
    std::cout << "mEntries.size = " << sizeof(mEntries) << std::endl;
    uint8_t *val1 = (uint8_t*) &haystack;
    std::cout << "----load mEntries-----:" << std::endl;
    printf("haystack = %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val1[0], val1[1], val1[2], val1[3], val1[4], val1[5],
        val1[6], val1[7], val1[8], val1[9], val1[10], val1[11], val1[12], val1[13], val1[14], val1[15],
        val1[16], val1[17], val1[18], val1[19], val1[20], val1[21], val1[22], val1[23], val1[24], val1[25],
        val1[26], val1[27], val1[28], val1[29], val1[30], val1[31]);
    #endif
    __m256i searchResult = _mm256_cmpeq_epi8(_mm256_and_si256(haystack, consideredBitsRegister), expectedBitsRegister);
    #ifdef PING_DEBUG3
    uint8_t *val3 = (uint8_t*) &searchResult;
    std::cout << "----searchResult-----:" << std::endl;
    printf("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i \n", val3[0], val3[1], val3[2], val3[3], val3[4], val3[5],
        val3[6], val3[7], val3[8], val3[9], val3[10], val3[11], val3[12], val3[13], val3[14], val3[15],
        val3[16], val3[17], val3[18], val3[19], val3[20], val3[21], val3[22], val3[23], val3[24], val3[25],
        val3[26], val3[27], val3[28], val3[29], val3[30], val3[31]);
    std::cout << "searchResult findMasksByPattern---function return = " << std::bitset<32>(_mm256_movemask_epi8(searchResult)) << std::endl;
    std::cout << "----findMasksByPattern---function-----finish" << std::endl;
    #endif
    return static_cast<uint32_t>(_mm256_movemask_epi8(searchResult));
  }

  template<>
  inline uint32_t SparsePartialKeys<uint16_t>::findMasksByPattern(__m256i consideredBitsRegister, __m256i expectedBitsRegister) const {
    #ifdef PING_DEBUG3
    printf("---New------SparsePartialKeys<uint16_t>::findMasksByPattern-------function starts----\n");
#endif
    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries)); //3 instr
    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16)); //4 instr

    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

    __m256i searchResult1 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack1, consideredBitsRegister), expectedBitsRegister);
    __m256i searchResult2 = _mm256_cmpeq_epi16(_mm256_and_si256(haystack2, consideredBitsRegister), expectedBitsRegister);

    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                 searchResult1, searchResult2
                                                                                 ), perm_mask);

    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
  }

  template<>
  inline uint32_t SparsePartialKeys<uint32_t>::findMasksByPattern(__m256i consideredBitsRegister, __m256i expectedBitsRegister) const {
#ifdef PING_DEBUG3
printf("---------SparsePartialKeys<uint32_t>::findMasksByPattern-------function starts----\n");
#endif
    __m256i haystack1 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries));
    __m256i haystack2 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 8));
    __m256i haystack3 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 16));
    __m256i haystack4 = _mm256_loadu_si256(reinterpret_cast<__m256i const *>(mEntries + 24)); //27 instr

    __m256i const perm_mask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0); //35 instr

    __m256i searchResult1 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack1, consideredBitsRegister), expectedBitsRegister);
    __m256i searchResult2 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack2, consideredBitsRegister), expectedBitsRegister);
    __m256i searchResult3 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack3, consideredBitsRegister), expectedBitsRegister);
    __m256i searchResult4 = _mm256_cmpeq_epi32(_mm256_and_si256(haystack4, consideredBitsRegister), expectedBitsRegister); //35 + 8 = 43

    __m256i intermediateResult = _mm256_permutevar8x32_epi32(_mm256_packs_epi16( //43 + 6 = 49
                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult1, searchResult2), perm_mask),
                                                                                 _mm256_permutevar8x32_epi32(_mm256_packs_epi32(searchResult3, searchResult4), perm_mask)
                                                                                 ), perm_mask);

    return static_cast<uint32_t>(_mm256_movemask_epi8(intermediateResult));
  }

  template<>
  inline __m256i SparsePartialKeys<uint8_t>::broadcastToSIMDRegister(uint8_t const mask) const {
    return _mm256_set1_epi8(mask);
  }

  template<>
  inline __m256i SparsePartialKeys<uint16_t>::broadcastToSIMDRegister(uint16_t const mask) const {
    return _mm256_set1_epi16(mask);
  }

  template<>
  inline __m256i SparsePartialKeys<uint32_t>::broadcastToSIMDRegister(uint32_t const mask) const {
    return _mm256_set1_epi32(mask);
  }

}}

                #endif
