// Copyright (c) 2015 Flowgrammable.org
// All rights reserved

// Trie is a tree where each vertex represents a word or prefix
// Trie here is used to find the longest prefix match in a routing table
// Matching on integral keys or bitstring of some type
// Arbitrary match: need to consider the wildcard in the key
// Wildcard has its value and masks



#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <tuple>
#include <utility>

using namespace std;

static uint64_t debug;
static uint64_t attempt;
static float insert_time;
static float search_time;
static bool blowup;
static uint32_t new_num1;
//static int blowup_num;  // how many wildcard *

// For arbitrary match, using format Rule(value, mask, priority) as input
// Changed uint32_t into uint64_t, including two fileds
struct Rule
{
  uint64_t value;
  uint64_t mask;
  uint32_t priority;
  uint32_t action;
//  double test;

  Rule()
  {
    value = 0;
    mask = 0;
    priority = 0;
    action = 0;
//    test = 0;
  }

  Rule(uint64_t x, uint64_t y, uint32_t z, uint32_t w)
  {
    value = x;
    mask = y;
    priority = z;
    action = w;
//    test = v;
  }

};

// Convert Rule(value, mask) into integer type in a trie data structure
// Guarantee using the same input format Rule(value, mask)
void convert_rule(vector<uint64_t>& rulesTable, Rule& rule);

void detect_wildcard_mask(Rule& rule);

// Construct a data structure for the result, which
// includes priority value and action value

struct trie_result
{
  uint32_t priority;
  uint32_t action;
  uint64_t cishu;

  trie_result()
  {
    priority = 0;
    action = 0;
    cishu = 0;
  }
};

struct trie_node
{
  uint32_t priority; // Used to mark rule nodes, and also can show pripority
  trie_node* children[2]; // Trie stride = 1, has two pointer, '0' and '1'
  uint32_t action;

  // trie_node constructor
  trie_node()
  {
    priority = 0;
    action = 0;
    children[0] = NULL;
    children[1] = NULL;
  }

};


// Create a new trie node
trie_node* new_node();

// Determine the node whether is an end node of a rule, according to the value
bool is_rule_node(trie_node* pNode);

// Determine the node whether is independent, whether has children or not
// If it is independent, which means it can be deleted
// If not, it cannot be deleted
bool is_independent_node(trie_node* pNode);

void removeChildren();

//class trie_node
//{
//public:

//  uint64_t priority; // Used to mark rule nodes, and also can show pripority
//  trie_node* children[2]; // Trie stride = 1, has two pointer, '0' and '1'
//  uint32_t action;

//  // trie_node constructor
//  trie_node()
//  {
//    priority = 0;
//    action = 0;
//    children[0] = NULL;
//    children[1] = NULL;
//  }

//  // Create a new trie node
//  trie_node* new_node();

//  // Determine the node whether is an end node of a rule, according to the value
//  bool is_rule_node(trie_node* pNode);

//  // Determine the node whether is independent, whether has children or not
//  // If it is independent, which means it can be deleted
//  // If not, it cannot be deleted
//  bool is_independent_node(trie_node* pNode);

//  void removeChildren();


//};

class Trie
{

public:

  // This is the input of rules (value, mask)
  trie_node* root;
  uint32_t count; // The number of rules in a trie
  uint64_t node_count; // The number of trie nodes in a trie
  uint64_t expand_count; // Make this variable global, the number of expanded rules

  vector<Rule> expandedTable;

  //uint32_t new_num = 0; // Make this variable global, can be access
  //vector<uint32_t> maskNewPosition;
  //int boundary = 0;

  // Trie constructor
  Trie()
  {
    root = new_node();
    count = 0;
    node_count = 1; // Include the root node
    expand_count = 0;

  }
  ~Trie()
  {
    // If use this destructor, the program has bugs, probably is double free
//    delete_trie(root);
  }

  // Delete a trie
  void delete_trie(trie_node *r);

  // Insert rules into the trie

  void insert_rule_value(uint64_t rule_value);

  void insert_prefix_rule(uint64_t value, uint64_t mask);

  void insert_rule(Rule& rule);

  uint32_t get_new_num(Rule& rule);

  // Expanding the arbitrary rules into prefix rules
  void expand_rule(Rule& rule);
  vector<Rule> tot_expand_rule(Rule& rule);

  int insert_prefix_rule_priority(Rule& rule);


  // Search the incoming packet in the trie
  // If return 0, match miss
  // If return 1, match hit
  bool search_rule(uint64_t key);

  bool prefix_search_rule(uint64_t key);
  bool LPM_search_rule(uint64_t key);
  trie_result LPM1_search_rule(uint64_t key);

  // New: delete the Rule format rules
  void resetRule(Rule& rule);

  // Delete rules in the trie
  // When deleting nodes, needs to satisfy two conditions
  // 1. the deleting node is not the end node of a rule ( is_rule_node() )
  // 2. the deleting node does not have childrens ( is_independent_node() )
  void delete_rule(uint64_t rule);

private:
  // Remove trie node in the trie
  // Helping delete rules
bool remove(trie_node *pNode, uint64_t rule, int level, int len);

};


