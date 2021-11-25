// Copyright (c) 2020 Ping
// All rights reserved


#include <iostream>
#include <vector>
#include <new>
#include <exception>
#include <string>
#include <limits.h>
#include <stdio.h>
#include <algorithm>
#include <chrono>
#include <time.h>
#include "basic_hot_tree.hpp"

using namespace std;



/*
 * Create a new hybrid node
 * for the whole 64-bit hot project, we just have one node for this hybrid_node type
 * for the root node of the whole trie
 */


hybrid_node* hot_trie::new_hybridNode()
{

  hybrid_node* pNode = NULL;
  pNode = new hybrid_node();
  if (pNode) {
    pNode->priority = 0;
    pNode->children[0] = NULL;
    pNode->children[1] = NULL;
  }
  return pNode;

}





