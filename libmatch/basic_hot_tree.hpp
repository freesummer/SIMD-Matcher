// Copyright (c) 2015 Flowgrammable.org
// All rights reserved

// Trie is a tree where each vertex represents a word or prefix
// Trie here is used to find the longest prefix match in a routing table
// Matching on integral keys or bitstring of some type
// Arbitrary match: need to consider the wildcard in the key
// Wildcard has its value and masks


//#define PING_DEBUG
//#define PING_DEBUG1 // for ====getNodeAllocationInformation function() starts======
//#define PING_DEBUG2 //getEntryDepths, collectEntryDepths
//#define PING_DEBUG3 // insert function starts and delete function indicators
//#define PING_DEBUG4 // match item debug info
//#define PING_DEBUG5

//#define PING_DEBUG9
//#define PING_DEBUG14

//#define PING_DEBUG11

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <tuple>
#include <utility>
#include <array>
using namespace std;

/*
 * the struct hybrid_node is used to add a big cap on the top of hot_tree
 * since we need to put the 63th bit (starting from 0th) into the hot 63-bit data structure
 * we will add a first layer for a single binary branch selection
 * then the search result getting from the binary branch will connect to the two hot_tree
 * 0-hot_tree and 1-hot_tree
 *
 * 07/03/2020 usage updates: so far I haven't use the priority and action member yet
 * maybe will use it later
 */



struct hybrid_node
{
  uint32_t priority; // Used to mark rule nodes, and also can show pripority
  intptr_t* children[2]; // Trie stride = 1, has two pointer, '0' and '1'
  uint32_t action;

  // hybrid_node constructor
  hybrid_node()
  {
    priority = 0;
    children[0] = 0;
    children[1] = 0;
    action = 0;
  }

};


//struct entryVector
//{
//  intptr_t value;
//  uint64_t mask;
//  uint64_t pp;

//  entryVector()
//  {
//    value = 0;
//    mask = 0;
//    pp = 0;
//  }
//};



class hot_trie
{

public:

  hybrid_node* root;
  uint32_t rule_count; // The number of rules in a trie
  uint64_t node_count; // The number of trie nodes in a trie

  // Create a new hybrid node
  hybrid_node* new_hybridNode();

  // hot_trie constructor
  hot_trie()
  {
    root = new_hybridNode();
    rule_count = 0;
    node_count = 1; // Include the root node

  }
  ~hot_trie()
  {

  }




};


