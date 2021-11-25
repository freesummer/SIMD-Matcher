// Copyright (c) 2015 Flowgrammable.org
// All rights reserved



#include "../libmatch/basic_hot_tree.cpp"
#include "../hot/single-threaded/include/hot/singlethreaded/HOTSingleThreaded.hpp"
#include "../hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedInterface.hpp"
#include "../hot/single-threaded/include/hot/singlethreaded/HOTSingleThreadedChildPointerInterface.hpp"
#include "../idx/content-helpers/include/idx/contenthelpers/IdentityKeyExtractor.hpp"
#include "../idx/content-helpers/include/idx/contenthelpers/OptionalValue.hpp"
#include "../idx/benchmark-helpers/include/idx/benchmark/Benchmark.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <array>
#include <chrono>
#include <algorithm>
#include <functional>
#include <set>
#include <time.h>
#include <map>
#include <cassert>

uint64_t match_count;
hot_trie hotTrie;

uint64_t max_mask;
uint64_t input_pp;  // priority input

/*
 * right shift count, since we have some wildcard rules, i.e. 1****, if the DB(4, 3, 2, 1), when compare, we just compare the non-wildcard bits,
 * it is not the all 4 bits in DB-bit set, it is just one bit: bit-4. so here need to right shirt by 3.
 */
int shr_count;

uint64_t wildRule_count;

uint32_t wildcard_affected_entry_mask;

uint64_t extract_mask;  // to get the extraction bit position

bool flag_mask; // flag for insert the wildcard rules, but the non-wildcard part are the same with the previous inserted rules, which triggered my "insertNewWildcardRules()" functions

int branch_index;
int T_left;
int T_right;
int key_index;  // for the key, indicate the most significant bit, if 1, search through mRoot_right data structure, 0, go to mRoot_left


using namespace std;


/**
 * Wrapper class to fulfill the requirements of the benchmarking framework
 */
class HotSingleThreadedIntegerBenchmarkWrapper {
  using TrieType = hot::singlethreaded::HOTSingleThreaded<uint64_t, idx::contenthelpers::IdentityKeyExtractor>;
  TrieType mTrie;

public:
  inline bool insert(uint64_t key) {
#ifdef PING_DEBUG3
    std::cout << "================benchmarkable.insert function============" << std::endl;
#endif
    hybrid_node* pRule = hotTrie.root;
    #ifdef PING_DEBUG3
    printf("hotTrie.root = %p\n", pRule);
    std::cout << "key = " << std::bitset<64>(key) << std::endl;
    #endif
    uint64_t new_value;
    new_value = key & (~(uint64_t(1) << 63));  // the 63-bit inserted into HOT data structure
    #ifdef PING_DEBUG3
    std::cout << "new_value = " << std::bitset<64>(new_value) << std::endl;
    #endif
    branch_index = (key >> 63) & uint64_t(1);
    #ifdef PING_DEBUG3
    std::cout << "branch_index = " << branch_index << std::endl;
    #endif
    if ( !pRule->children[branch_index] ) {
      //  the index-branch is Null now, need to create a new hybrid node
      #ifdef PING_DEBUG3
      std::cout << branch_index << "-branch is empty" << std::endl;
      #endif
      pRule->children[branch_index] = (intptr_t*)hotTrie.new_hybridNode();
      /*
       * maybe need to force change to the hybrid_node type,
       * in order to pass the pRule pointer
       */
      if (branch_index == 0){
        T_left = 1;
      }
      if (branch_index == 1) {
        T_right = 1;
      }
      hotTrie.node_count++; // Created a new node in the hot_trie
    }
    else {
      //  the branch is already here
      if (branch_index == 0){
        T_left = 0;
      }
      if (branch_index == 1) {
        T_right = 0;
      }
    }
    pRule = (hybrid_node*)pRule->children[branch_index];  // did some type transform in order to fulfill the type

    return mTrie.insert(new_value);
  }

  /*
   * new insert function with two fileds: key and mask
   * type: uint64_t
   */

  inline bool insert(uint64_t key, uint64_t mask) {
#ifdef PING_DEBUG3
    std::cout << "================benchmarkable.insert function============" << std::endl;
#endif
    hybrid_node* pRule = hotTrie.root;
    #ifdef PING_DEBUG3
    printf("hotTrie.root = %p\n", pRule);
    std::cout << "key = " << std::bitset<64>(key) << std::endl;
    #endif
    uint64_t new_value;
    new_value = key & (~(uint64_t(1) << 63));  // the 63-bit inserted into HOT data structure
    #ifdef PING_DEBUG3
    std::cout << "new_value = " << std::bitset<64>(new_value) << std::endl;
    #endif
    branch_index = (key >> 63) & uint64_t(1);
    #ifdef PING_DEBUG3
    std::cout << "branch_index = " << branch_index << std::endl;
    #endif
    if ( !pRule->children[branch_index] ) {
      //  the index-branch is Null now, need to create a new hybrid node
      #ifdef PING_DEBUG3
      std::cout << branch_index << "-branch is empty" << std::endl;
      #endif
      pRule->children[branch_index] = (intptr_t*)hotTrie.new_hybridNode();
      /*
       * maybe need to force change to the hybrid_node type,
       * in order to pass the pRule pointer
       */
      if (branch_index == 0){
        T_left = 1;
      }
      if (branch_index == 1) {
        T_right = 1;
      }
      hotTrie.node_count++; // Created a new node in the hot_trie
    }
    else {
      //  the branch is already here
      if (branch_index == 0){
        T_left = 0;
      }
      if (branch_index == 1) {
        T_right = 0;
      }
    }
    pRule = (hybrid_node*)pRule->children[branch_index];  // did some type transform in order to fulfill the type

    return mTrie.insert(new_value, mask);
  }


  inline bool remove(uint64_t key) {
#ifdef PING_DEBUG
    std::cout << "benchmarkable.remove" << std::endl;
#endif
    return mTrie.remove(key);
  }

  inline bool search(uint64_t key) {
    /* need to add the first branch part
     * process the key from 64-bit to 63-bit using the hot_tree data structure
     */
#ifdef PING_DEBUG3
    std::cout << "========benchmarkable.search=======" << std::endl;
#endif
#ifdef PING_DEBUG3
    std::cout << "    key = " << std::bitset<64>(key) << std::endl;
  #endif
    uint64_t new_key = key & (~(uint64_t(1) << 63));  // the 63-bit inserted into HOT data structure
    #ifdef PING_DEBUG3
    std::cout << "new_key = " << std::bitset<64>(new_key) << std::endl;
      #endif
    key_index = (key >> 63) & uint64_t(1);
    #ifdef PING_DEBUG3
    std::cout << "key_index = " << key_index << std::endl;
 #endif
    idx::contenthelpers::OptionalValue<uint64_t> result = mTrie.lookup(new_key);

#ifdef PING_DEBUG3
    std::cout << "result.mValue = " << std::bitset<64>(result.mValue) << std::endl;
    std::cout << "result.Rmask = " << result.Rmask << std::endl;
    std::cout << "new_key       = " << std::bitset<64>(new_key) << std::endl;
    std::cout << "result.mIsValid = " << result.mIsValid << ", (result.mValue ?= key) = " << ( (result.mValue | result.Rmask) == (new_key | result.Rmask) ) << std::endl;
    printf("search result = %d\n", result.mIsValid & ( (result.mValue | result.Rmask) == (new_key | result.Rmask) ));
#endif

    bool tmp = result.mIsValid & ( (result.mValue | result.Rmask) == (new_key | result.Rmask) );

        if (tmp) {
          // find a match
          match_count += 1;
          //std::cout << "find a match: match = " << tmp << std::endl;
          return tmp;
        }

        return false;

  }

  inline bool iterateAll(std::vector<uint64_t> const & iterateKeys) {
#ifdef PING_DEBUG3
    std::cout << "========benchmarkable.iterateAll=======" << std::endl;
    std::cout << "iterateKeys.size = " << iterateKeys.size() << std::endl;
#endif
    size_t i=0;
    bool iteratedAll = true;
    for(uint64_t value : mTrie) {
      iteratedAll = iteratedAll & (value == iterateKeys[i]);
#ifdef PING_DEBUG3
    std::cout << "value = "  << std::bitset<64>(value) << std::endl;
    std::cout << "i = "  << i << ", iterateKeys[i] = "<< std::bitset<64>(iterateKeys[i]) << std::endl;
#endif
      ++i;
    }
    return iteratedAll & (i == iterateKeys.size());
  }

  idx::benchmark::IndexStatistics getStatistics() {
    std::pair<size_t, std::map<std::string, double>> stats = mTrie.getStatistics();
    return { stats.first, stats.second };
  }

  idx::benchmark::IndexStatistics getStatistics_left() {
    std::pair<size_t, std::map<std::string, double>> stats = mTrie.getStatistics_left();
    return { stats.first, stats.second };
  }


  idx::benchmark::IndexStatistics getStatistics_right() {
    std::pair<size_t, std::map<std::string, double>> stats = mTrie.getStatistics_right();
    return { stats.first, stats.second };
  }




};


/*
 * this function process the 64-bit into 63-bit integer since HOT just support upto 63-bitset
 * input: 64-bit integer
 * output: 63-bit integer
 * the type is still the same, uint64_t, just move out of the highest bit 63-bit, starting from 0-bit
 * the removed 63-bit will be inserted into a binary tree, or the bucket data structure, 0-branch and 1-branch
 */
uint64_t process_63_bit(uint64_t rule_value)
{
  uint64_t new_value;
  new_value = rule_value & (~(uint64_t(1) << 63));
  return new_value;

}




int main(int argc, char* argv[])
{
#ifdef PING_DEBUG3
  printf("====Simulation=====****Run****==========start============================\n");
  printf("hotTrie.root = %p, hotTrie.rule_count = %d , hotTrie.node_count = %lu\n", hotTrie.root, hotTrie.rule_count, hotTrie.node_count);
#endif
  idx::benchmark::Benchmark<HotSingleThreadedIntegerBenchmarkWrapper> benchmark(argc, argv, "HotSingleThreadedIntegerBenchmark");
  benchmark.run();

  std::cout << "match_count: " << match_count << std::endl;
  std::cout << "wildRule_count: " << wildRule_count << std::endl;
  #ifdef PING_DEBUG3
  printf("====Simulation=====****Run****==========ends************============================\n");
  #endif
  return 0;
}






