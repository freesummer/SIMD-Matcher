#ifndef __IDX__UTILS__8ByteDatFileIO__
#define __IDX__UTILS__8ByteDatFileIO__

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <set>
#include <vector>

//#include "libmatch/basic_trie.hpp"

extern uint64_t max_mask;

namespace idx { namespace utils {



inline bool mySortFunction (std::pair<uint64_t, uint64_t> pair1, std::pair<uint64_t, uint64_t> pair2)
{
  return (pair1.second < pair2.second);
}

/*
 * New function: readDatFile_pair()
 * changed the dataFile from type vector<uint64_t> to vector<Rule>
 * pair<uint64_t,uint64_t>
 * pair.first, pair.second
 */
inline std::vector<std::pair<uint64_t, uint64_t>> readDatFile_pair(std::string const &fileName, size_t numberValues = 0) {
  //std::cout << "vector<uint64_t> readDatFile" << std::endl;
  size_t recordsToRead;
  //std::cout << "numberValues = "  << numberValues << std::endl;
  std::ifstream input(fileName);
  //  reading the data: currentValue
  input >> recordsToRead; // read the first integer in the input file. which = recordsToRead
  if (numberValues > 0) {
    recordsToRead = std::min(recordsToRead, numberValues);
  }
  std::cout << "readDatFile_pair function: recordsToRead = " << recordsToRead << std::endl;
  //  std::vector<uint64_t> values;
  std::vector<std::pair<uint64_t, uint64_t>> values;
  values.reserve(recordsToRead);
  std::set<std::pair<uint64_t, uint64_t>> duplicateChecker;
  int numberDuplicates = 0;
  for (size_t i = 0; i < recordsToRead; ++i) {
    std::pair<uint64_t, uint64_t> currentPair;
    currentPair.first = 0, currentPair.second = 0;
    input >> currentPair.first;
    input >> currentPair.second;
#ifdef PING_DEBUG3
    printf("i = %zu, currentPair.value = %lu, currentPair.mask = %lu\n", i, currentPair.first, currentPair.second);
#endif
    if(duplicateChecker.find(currentPair) != duplicateChecker.end()) {
      ++numberDuplicates;
      //printf("duplicate....\n");
    } else {
      duplicateChecker.insert(currentPair);
      values.push_back(currentPair);
      if (currentPair.second > max_mask) {
        max_mask = currentPair.second;
      }
    }
  }
  /*
   * sort the vector:value, by the pair.second, mask values
   */

//  std::sort(values.begin(), values.end(), mySortFunction);
#ifdef PING_DEBUG3
  for (uint i = 0; i < values.size(); i++) {
    std::cout << values[i].first << " " << values[i].second << std::endl;
  }
#endif

  input.close();
  //printf("duplicateChecker.size = %d\n", duplicateChecker.size());
  std::cout << "Rule set: "<< fileName << ".size (non-duplicate) = " << values.size() << std::endl;
  return std::move(values);
}





inline std::vector<uint64_t> readDatFile(std::string const &fileName, size_t numberValues = 0) {
  //std::cout << "vector<uint64_t> readDatFile" << std::endl;
  size_t recordsToRead;
  //std::cout << "numberValues = "  << numberValues << std::endl;
  std::ifstream input(fileName);
  //  reading the data: currentValue
  input >> recordsToRead; // read the first integer in the input file. which = recordsToRead
  if (numberValues > 0) {
    recordsToRead = std::min(recordsToRead, numberValues);
  }
  std::cout << "recordsToRead = " << recordsToRead << std::endl;
  std::vector<uint64_t> values;
  values.reserve(recordsToRead);
  std::set<uint64_t> duplicateChecker;
  int numberDuplicates = 0;
  for (size_t i = 0; i < recordsToRead; ++i) {
    uint64_t currentValue = 0;
    //  reading the data: currentValue
    input >> currentValue;

    if(duplicateChecker.find(currentValue) != duplicateChecker.end()) {
      ++numberDuplicates;
      //printf("duplicate....\n");
    } else {
      duplicateChecker.insert(currentValue);
      values.push_back(currentValue);
    }
  // remove the duplicate key value
//    values.push_back(currentValue);

  }
  input.close();
  //printf("duplicateChecker.size = %d\n", duplicateChecker.size());
  std::cout << "Key set: " << fileName << ".size (non-duplicate) = " << values.size() << std::endl;
  return std::move(values);
}

inline void readDatFile(std::string const &fileName, uint64_t* values, size_t numberValues) {
  std::cout << "void readDatFile" << std::endl;
  size_t recordsToRead;
  std::ifstream input{fileName};
  input >> recordsToRead;
  if (numberValues < recordsToRead) {
    std::cout << "Should read " << numberValues << " values but " << fileName << " only contains " << recordsToRead << " values."<< std::endl;
    exit(1);
  }
  std::cout << "Reading " << numberValues << " values " << std::endl;
  for (size_t i = 0; i < numberValues; ++i) {
    input >> values[i];
  }
  input.close();
}


inline void writeDataFile(std::ostream & output, std::vector<uint64_t> const &values) {
  output << values.size() << std::endl;
  for(uint64_t value : values) {
    output << value << std::endl;
  }
}

//maybe switch to exceptions, exit(1) is not realy nice
inline void writeDatFile(std::string const &fileName, std::vector<uint64_t> const &values) {
  std::ofstream output{fileName};
  if (!output.is_open()) {
    output << "Could not write data file to " << fileName << std::endl;
    exit(1);
  }
  writeDataFile(output, values);
  output.close();
}


} }

#endif
