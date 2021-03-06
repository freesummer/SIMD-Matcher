// Copyright (c) 2015 Flowgrammable.org
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
#include "basic_trie.hpp"

using namespace std;
extern char * _eMemory;
static uint64_t threshold;

using  ns = chrono::nanoseconds;
using  ms = chrono::microseconds;
using s = chrono::seconds;
using get_time = chrono::steady_clock;

// This function is caculating the log2 value
static uint64_t mylog2 (uint64_t val) {
  if (val == 0) return UINT_MAX;
  if (val == 1) return 0;
  uint64_t ret = 0;
  while (val > 1) {
    val >>= 1;
    ret++;
  }
  return ret;
}


// Convert the input rule format Rule(value, mask) into the integer type in trie
void convert_rule(vector<uint64_t>& rulesTable, Rule& rule)
{
  // Store the wildcard postion into vector maskPosion
  vector<uint32_t> maskPosition;
  // Check the mask field from the lower bit
  for(int i = 0; i < 64; i++) {
    // if this: get the position whose bit is 1 (have wildcard)
    if((rule.mask >> i) & 1 == 1) {
      maskPosition.push_back(i);
    }
  }
  uint32_t num = maskPosition.size(); // num is the number of wildcard
  uint64_t base = rule.value & (~rule.mask); // Get the value field of each rule
  for(int i = 0; i < (1 << num); i++) {
    uint64_t expandedRule = base; // This is the base rule, smallest integer rule
    for(int j = 0; j < num; j++) {
      if(((1 << j) & i) == 0) {
        // get the expandedRule for ruleTables
        expandedRule |= (1 << maskPosition.at(j));
      }
    }

    rulesTable.push_back(expandedRule);
  }
}

// Create a new trie node
trie_node* new_node()
{
  trie_node* pNode = NULL;
  pNode = new trie_node();
  debug++; // add 1

  //  try{
  //    pNode = new trie_node();
  //  }
  //  catch (std::bad_alloc& ba)
  //  {
  //    cerr << "bad allocation: " << ba.what() << endl;
  //    exit(EXIT_FAILURE);
  //  }
  if (pNode) {
    pNode->priority = 0;
    pNode->children[0] = NULL;
    pNode->children[1] = NULL;
  }
  return pNode;
}

// Determine the node whether is leaf node
// If the priority = 0, it is not leaf node
// If the priority != 0, it is leaf node
bool is_rule_node(trie_node *pNode)
{
  if (pNode->priority == 0) {
    return false;
  }
  else {
    return true;
  }
  // If the priority != 0, it is a rule ending node
}

// Determine whether a node has children or not
// If the node has no children, then it is an independent node
// If the node has children, it is not
bool is_independent_node(trie_node *pNode)
{
  if ( (pNode->children[0] == NULL) && (pNode->children[1] == NULL) ) {
    return true;
  }
  else {
    return false;
  }
}


void Trie::delete_trie(trie_node *r)
{
  if (r == NULL) {
    return;
  }
  else {
    if (r->children[0] != NULL ) {
      delete_trie(r->children[0]);
    }
    if ( r->children[1] != NULL) {
      delete_trie(r->children[1]);
    }
    delete r;
    debug = debug - 1;
    //cout << "test test" << endl;
    r = NULL;
  }


}

// Insert rules: original insert function, without expanding ** part.
void Trie::insert_rule_value(uint64_t rule_value)
{
  int level;
  int index;
  trie_node* pRule;
  count++;
  pRule = root; // Created the first node in a trie
  // Here is 64, because of the fixed length uint64_t
  for (level=0; level<64; level++) {
    // Get the index value of each bit, totally is 32
    index = (rule_value >> (63-level)) & 1;
    // if the key is not present in the trie (is NULL), insert a new node
    if ( !pRule->children[index] ) {
      pRule->children[index] = new_node();
      node_count++; // Created a new node in a trie
    }
    pRule = pRule->children[index];
  }
  pRule->priority = count; // If the priority is not 0, the node is leaf node

}

// Insert rules with Rule(value, mask)
// Expand wildcard part in a Rule(value, mask)
void Trie::insert_rule( Rule& rule )
{
  // Store the wildcard postion into vector maskPosion
  vector<uint32_t> maskPosition;
  // Check the mask field from the lower bit
  for(int i = 0; i < 64; i++) {
    // if this: get the position whose bit is 1 (have wildcard)
    if((rule.mask >> i) & 1 == 1) {
      maskPosition.push_back(i);
    }
  }
  uint32_t num = maskPosition.size(); // num is the number of wildcard
  uint64_t base = rule.value & (~rule.mask); // Get the value field of each rule
  for(int i = 0; i < (1 << num); i++) {
    uint64_t expandedRule = base; // This is the base rule, smallest integer rule
    for(int j = 0; j < num; j++) {
      if(((1 << j) & i) == 0) {
        // get the expandedRule for ruleTables
        expandedRule |= (1 << maskPosition.at(j));
      }
    }
    insert_rule_value(expandedRule);

  }

}

// Get the "*" wildcard number after the multi-prefix number
// which means the number of wildcard will be expand
// 2^ (number of wildcard) = number of expanded rules
uint32_t Trie::get_new_num(Rule& rule)
{
  int boundary = 0;
  for (int i=0; i<64; i++) {
    // Find the first bit "0" from the least significant bit
    // 10x1xx, so the mask is 001011
    if ( ((rule.mask >> i) & uint64_t(1)) == 0 ) {
      boundary = i;
      break;
      //cout << "boundary is" << " " << boundary << endl;
    }
    else {
      continue;
    }
  }
  // Find the first "0" bit, which is the arbitrary bit in the middle
  // Then detect all the "1" bit, which means all the wildcard in non-prefix positions
  // Need to expand all wildcard to make the rules become prefix rules
  vector<uint32_t> maskNewPosition; // To store the expand wildcard position
  for (int j=(boundary+1); j<64; j++) {
    if ( ((rule.mask >> j) & uint64_t(1)) == 1 ) {
      maskNewPosition.push_back(j); // recored the positions that should be expanded (the bit is "1")
      continue; // record all the candidates in 64-bit
    }
    else {
      continue; // check all the 64-bit
    }
  }
  // Get all the "1" in the new rule, besides the prefix part
  // need to expand all the "1" part
  uint32_t new_num = maskNewPosition.size(); // num is the number of wildcard
  //cout << new_num << endl;
  return new_num;
}

/* modied at 03/31/2020 ====> modify 64-bit to 32-bit
 * in order to compare with the HOT performance
 * modied back to 64-bit @ 06/02/2020
 * modied back to 32-bit @ 06/09/2020
 * modied back to 64-bit @ 06/15/2020
 */
int Trie::insert_prefix_rule_priority( Rule& rule )
{
  int test_error = 0;
  trie_node* pRule = root;
  // Considering the prefix rules, thus the length of rule should be different
  // depends on the mask value, the length of each rule = 32-mask
  // The length of wildcard = mask number
  uint32_t mask_num = mylog2(rule.mask+1);
  // Has a bug here: when 32 bits are all wildcard, will overflow
  uint32_t prefix_len = 64 - mask_num;  // modied at 03/32/2020    64 ====> 32
    auto start777 = get_time::now();
  //  clock_t t;
  //  t = clock();
  for (int level = 0; level < prefix_len; level++) {
    // Get the index value of each bit, totally is 32
    int index = (rule.value >> (63-level)) & uint64_t(1); // modied at 03/32/2020 63 ====> 31
    // if the key is not present in the trie (is NULL), insert a new node
    if ( !pRule->children[index] ) {
      if (node_count + 1 > threshold){
        test_error = 1;
        //cout << "Error: bad_allo memory" << endl;
        return test_error;
      }
      pRule->children[index] = new_node();
      node_count++; // Created a new node in a trie
    }
    // move to child node:
    pRule = pRule->children[index];
    // check if rule exists:
    if (pRule->priority > 0 && pRule->priority < rule.priority) {
      // If this trie node has a rule, and the priority is higher, do not insert rule
      // probably print a message of interest
      //      cout << "check priority ===========" << endl;
      return test_error;
    }
  }
  //  t = clock() - t;
  //  insert_time += ((float)t)/CLOCKS_PER_SEC;
    auto end777 = get_time::now();
    auto diff777 = end777 - start777;
  insert_time += chrono::duration_cast<ns>(diff777).count();
  // Insert the new rule:
  count++;
  pRule->priority = rule.priority; // If the priority is not 0, the node is leaf node
  pRule->action = rule.action;

  return test_error;
}


void Trie::expand_rule( Rule& rule )
{
  int boundary1 = 0;
  for (int i = 0; i < 64; i++) {
    // Find the first bit "0" from the least significant bit
    // 10x1xx, so the mask is 001011
    if ( ((rule.mask >> i) & uint64_t(1)) == 0 ) {
      boundary1 = i;
      break;
    }
    else {
      continue;
    }
  }
  vector<uint32_t> maskNewPosition1;
  for (int j=(boundary1+1); j<64; j++) {
    if ( ((rule.mask >> j) & uint64_t(1)) == 1 ) {
      maskNewPosition1.push_back(j); // recored the positions that should be expanded (the bit is "1")
      continue; // record all the candidates in 64-bit
    }
    else {
      continue; // check all the 64-bit
    }
  }
  // Get all the "1" in the new rule, besides the prefix part
  // need to expand all the "1" part
  new_num1 = maskNewPosition1.size(); // num is the number of wildcard
  //  cout << "=====***" << endl;
  //  cout << "WILDCARD # is " << new_num1 << endl;
  if (new_num1 > 15) {
    blowup = true;
    return;
  }

  Rule expandedRule;
  //  vector<Rule> expandedPingRulesTable;

  // From the priority = 40 rule, starts having wildcard, not the prifix rules,
  // need to be expanded

  for(uint64_t i = 0; i < ( uint64_t(1) << new_num1 ); i++) {

    expandedRule.value = rule.value; // base value
    // Show the last expandedRule value
    // expand into the number of rules: depending on the number of wildcard
    for(int j = 0; j < new_num1; j++) {
      // Get the weight on all the new_num bit
      // which produce every conbination
      // ex. for the first combination value is "0"
      // so on each new_num bit, the weight are all 0
      // ex. for the second conbination value is "1"
      // then the "00001", so the weight of the first bit of new_num bits is (uint64_t(1) << 0)
      if(((uint64_t(1) << j) & i) != 0) {
        // get the expandedRule for ruleTables
        expandedRule.value |= (uint64_t(1) << maskNewPosition1.at(j));
      }
    }
    expandedRule.mask = ( uint64_t(1) << boundary1 ) - 1; // mask value should be a prefix value after expanded
    expandedRule.priority = rule.priority; // priority keeps the same
    expandedRule.action = rule.action;

    int test = insert_prefix_rule_priority(expandedRule);
    if (test == 1) {
      return;
    }
    expandedTable.push_back(expandedRule);
    expand_count += expandedTable.size();

  }


}





// Added action attribute
// Insert prefix rules with Rule(value, mask, priority, action)
// Added the priority purpose
// Inserting rules will depend on the priority
// The priority must be in-order of insertion
// Which can reduce the number of inserted rules, make the data structure smaller



// Insert prefix rules with Rule(value, mask)
void Trie::insert_prefix_rule(uint64_t value, uint64_t mask)
{
  int level;
  int index;
  trie_node* pRule;
  count++;
  pRule = root;
  // Considering the prefix rules, thus the length of rule should be different
  // depends on the mask value, the length of each rule = 32-mask
  // The length of wildcard = mask number

  uint32_t mask_num = mylog2(mask+1);
  uint32_t layer = 64 - mask_num;

  for (level=0; level<layer; level++) {
    // Get the index value of each bit, totally is 32
    index = (value >> (63-level)) & 1;
    // if the key is not present in the trie (is NULL), insert a new node
    if ( !pRule->children[index] ) {
      pRule->children[index] = new_node();
    }
    pRule = pRule->children[index];
  }
  pRule->priority = count; // If the priority is not 0, the node is leaf node

  //cout << pRule->priority << " " << pRule->star_num << endl;
}


// Prefix rules lookup--search rules

bool Trie::LPM_search_rule(uint64_t key)
{
  trie_node* pRule;
  int index;  // The index of children, here just has 2 children
  int level;
  pRule = root;

  for (level=0; level<64; level++) {
    index = (key >> (63-level)) & 1;

    if ( !pRule->children[index] && pRule->priority == 0) {
      return 0;
    }
    if ( !pRule->children[index] && pRule->priority != 0) {
      return 1;
    }
    if ( pRule->children[index] && pRule->children[index]->priority == 0) {
      return 1;
    }
    if ( pRule->children[index] && pRule->children[index]->priority != 0) {
      pRule = pRule->children[index];
      continue;
    }
  }

  return false;
  // If return 0, match miss
  // If return 1, match hit
}

// Prefix rules lookup--search rules
// Assume the priority is more significant than the longest prefix match (not sure is correct)????
// Return the match priority, show which rule is being matched
// Using this search function, change to cpu time
/* modied at 03/31/2020 ====> modify 64-bit to 32-bit
 * in order to compare with the HOT performance
 * modied back to 64-bit @ 06/02/2020
 * modied back to 32-bit @ 06/09/2020
 * modied back to 64-bit @ 06/15/2020
 *
 */
trie_result Trie::LPM1_search_rule(uint64_t key)
{
  trie_node* pRule = root;

  trie_result ret;
  vector<uint64_t> priorityArray;
  //  vector<uint64_t>::iterator it;
  //  vector<uint32_t> actionArray;

  for (int level = 0; level < 64; level++) {  // modied at 03/31/2020    64=====>32
    int index = (key >> (63 - level)) & uint64_t(1);  // modied at 03/31/2020    63=====>31
    // Choose child based on relelvant bit in key.
    // If child exists, recurse:
    pRule = pRule->children[index];
    if ( pRule ) {
      // Node is a rule, save match:
      if ( pRule->priority != 0 ) {
        // If matches, insert the priority value into the matchArray vector
        priorityArray.push_back(pRule->priority);
        //        actionArray.push_back(pRule->action);
        ret.priority = *min_element(priorityArray.begin(), priorityArray.end());
      }
    }
    else {
      // Else, child does't exist. Stop and return last match.
      break;
    }
    ret.cishu = level + 1; // The number of match attempts
  }

  return ret;


  //  it = find(priorityArray.begin(), priorityArray.end(),ret.priority);
  //  int position = distance(priorityArray.begin(), it);

  //  if (priorityArray.size() == 0) {
  //    ret.action = 0;
  //    ret.priority = 0;
  //  }

  //  else {
  //    ret.action = actionArray.at(position);
  //  }

}


// Lookup--search rules
bool Trie::search_rule(uint64_t key)
{
  int level;
  int index;  // The index of children, here just has 2 children
  trie_node* pRule;
  pRule = root;
  for (level=0; level<64; level++) {
    index = (key >> (63-level)) & 1;
    if ( !pRule->children[index] ) {
      return 0;
    }
    pRule = pRule->children[index];
  }
  // If return 0, match miss
  // If return 1, match hit
  return (0 != pRule && pRule->priority);

}

void Trie::resetRule(Rule& rule)
{
  rule.value = 0;
  rule.mask = 0;
  rule.priority = 0;
}


bool Trie::remove(trie_node* pNode, uint64_t rule, int level, int len)
{
  int index;
  if(pNode) {
    if(level == len) {
      pNode->priority = 0; // Unmark leaf node
      // If this node has no children, can be deleted
      if(is_independent_node(pNode)) {
        return true;
      }
      else {
        return false;
      }
    }
    else {
      index = (rule >> (63-level)) & 1;
      if(remove(pNode->children[index], rule, level+1, len)) {
        delete pNode->children[index]; // Last node marked, delete it
        pNode->children[index] = NULL;
        // Determine whether the upper nodes should be deleted, parent node
        return(!is_rule_node(pNode) && is_independent_node(pNode));
      }
    }
  }
  return false;
}

// Delete rules
void Trie::delete_rule(uint64_t rule)
{
  int len = 64;
  if( len > 0 ) {
    remove(root, rule, 0, len);
  }
}

