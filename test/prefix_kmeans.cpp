// Copyright (c) 2015 Flowgrammable.org
// All rights reserved

#include "../libmatch/basic_trie.cpp"

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <chrono>
#include <algorithm>
#include <functional>
#include <set>
#include <time.h>

using namespace std;
using  ns = chrono::nanoseconds;
using  ms = chrono::microseconds;
using get_time = chrono::steady_clock ;

static uint64_t delta;  // the converge indicator
static int cluster_num;
int converge_time = 0;

vector< vector <Rule> > target;

ofstream outputFile("lib_ruleS14096.txt");
ofstream outputFile_keys("22lib_rule7026_small_keys_trans.txt");

struct Result {
  // define the result struct for merging algorithm
  int flag = -1; // show the different bit position
  int dif = 0; // the number bit of difference
};

struct vector_vec_result {
  vector< vector<Rule> > result1;
  vector< vector<uint64_t> > result2;
};

struct Return_vec {
  // define the return result for build_tries() function
  int exceed_flag = 0;
  bool flag = false;
  uint64_t size_trienode = 0;
  vector<Trie> tries;
  vector< vector<int> > delta;
  uint64_t expand_count = 0;
  float rule_conf_time;
  int insert_num_rule;
  vector<Rule> expandrules;
  vector<Rule> prerules;
  vector<int> expand_index;
  vector<int> prefix_index;
};

struct Search_result {
  double sousuo_time;
  float conf_time;
  uint64_t actionSum;
  uint64_t checksum;
  uint64_t match;
  float decision_time;
  vector<int> matched_priority;
  vector<int> vv;
};

// Matches integer type
Rule strTint(string rulestr)
{
  Rule rule;
  rule.mask = 0;
  rule.value = 0;
  // Find the position of ".", which spilit IP address to 4 parts
  size_t p1 = rulestr.find_first_of(" ");

  string substr1 = rulestr.substr(0,p1);
  string substr2 = rulestr.substr((p1 + 1));


  rule.value = stoull(substr1); // unsigned long long 64bit
  rule.mask = stoull(substr2);

  return rule;
}




/*
 * Check whether the new rule is a prefix rule
 * if yes, do the LPM insertion
 * if not, do the expand rule function
*/
bool is_prefix(Rule& rule)
{
  if (rule.mask == 0) {
    return true;
  }
  // Store the wildcard postion into vector maskPosion
  vector<uint32_t> maskPosition;
  // Check the mask field from the lower bit
  for(int i = 0; i < 64; i++) {
    // if this: get the position whose bit is 1 (have wildcard)
    if((rule.mask >> i) & uint64_t(1) == 1) {
      maskPosition.push_back(i);
    }
  }
  uint32_t num = maskPosition.size(); // num is the number of wildcard
  if (rule.mask == (uint64_t(1) << num)-1) {
    return true;
  }
  else {
    return false;
  }
}

vector< pair <uint64_t, uint64_t> > pair_function(vector<Rule>& ruleList)
{
  vector< pair <uint64_t, uint64_t> > pair_vector;
  for (uint64_t j = 0; j < 64; j++) {
    uint64_t score = 0;
    for (uint64_t i = 0; i < ruleList.size(); i++) {
      score = score + ((((ruleList.at(i)).mask) >> j) & uint64_t(1));
    }
    pair_vector.push_back(make_pair(j, score));
  }

  return pair_vector;

}


bool com_pair(pair <uint64_t, uint64_t> i, pair <uint64_t, uint64_t> j)
{
  if (i.second > j.second) return true;
  if (i.second == j.second) return i.first < j.first;
  return false;

}



vector<int> my_pairsort(vector< pair <uint64_t, uint64_t> > pairVector)
{
  vector< pair <uint64_t, uint64_t> > new_pair(pairVector);
  sort(new_pair.begin(), new_pair.end(), com_pair);
  vector<int> delta;
  // checkpoint is the index/subscript of the newSumColumn has the same value with the sumColumn
  for (int i = 0; i < pairVector.size(); i++) {
    for (int j = 0; j < new_pair.size(); j++) {
      // Check the first equal element, if it is true, then return the checkpoint
      if (new_pair[j] == pairVector[i]) {
        delta.push_back(i-j);
        break;
      }
    }

  }
  return delta;
}


/*
 * Generate the two dimensional array (generate delta array) from the pingRulesTable array
 * With all the bits in each rule, including "0" and "1"
 * Caculate the score--the sum of column
*/
vector<int> generate_delta(vector<Rule>& ruleList)
{
  vector<uint64_t> sumColumn;
  for (uint64_t j = 0; j < 64; j++) {
    uint64_t score = 0;
    for (uint64_t i = 0; i < ruleList.size(); i++) {
      score += ((((ruleList.at(i)).mask) >> j) & uint64_t(1));
    }
    sumColumn.push_back(score);
  }

  // Copy the sumColumn vector to a new vector
  vector<uint64_t> newSumColumn(sumColumn);

  // Sort the newSumColumn vector in descending order
  std::sort(newSumColumn.begin(), newSumColumn.end(), std::greater<uint64_t>());

  // Construct the delta(): the rearrangement operation {left shift, right shift, and no change}
  // the element in delta vector, can be negative, positive and "0"
  vector<int> delta;
  // checkpoint is the index/subscript of the newSumColumn has the same value with the sumColumn
  int checkpoint = 0;
  int gap = 0; // Gap is the difference between the original and new sumcolumn vectors
  for (int i = 0; i < sumColumn.size(); i++) {
    for (int j = 0; j < newSumColumn.size(); j++) {
      // Check the first equal element, if it is true, then return the checkpoint
      if (newSumColumn[j] != sumColumn[i]) {
        continue;
      }
      else if (newSumColumn[j] == sumColumn[i]) {
        checkpoint = j;
        newSumColumn[j] = 132; // make the matched column invalid
        break;
      }
      else {
        // Search all the 64 values, still didn't find the same value
        cout << "Error occurs" << endl;
      }
    }
    // Get the difference between the original vector data and the sorted vector data
    // Create the rearrangement operation
    gap = i - checkpoint;
    delta.push_back(gap);
  }
  return delta;
}

/*
 * Generate the new rule after the delta operations
 * if the element value of delta vector is negative, which means left shift
 * if the element value of delta vector is positive, which means right shift
 * if the element value of delta vector is "0", which means no change
*/
// Create a new pingRulesTable for the new rearrangement rules

vector<Rule> rules_rearrange(vector<Rule>& oldRuleList, vector<int> delta_array)
{
  vector<Rule> sumRulesTable; // for a new rule

  for (uint64_t i = 0; i < oldRuleList.size(); i++) {
    Rule newRule;
    for (uint64_t j = 0; j < 64; j++) {
      Rule subRule;
      if (delta_array[j] < 0) {
        // if it is negative, do the left shift
        // from the lsb position, do the correct operations
        // Note: because here is 64 bit, if we just use 1 to do left shift, it will occur overflow
        // since "1" is 32bit by default
        subRule.value = ( (( (oldRuleList[i].value) & (uint64_t(1) << j) ) ) << (abs(delta_array[j])) );
        subRule.mask = ( (( (oldRuleList[i].mask) & (uint64_t(1) << j) ) ) << (abs(delta_array[j])) );
      }
      else if (delta_array[j] > 0) {
        // if it is positive, do the right shift
        subRule.value = ( (( (oldRuleList[i].value) & (uint64_t(1) << j) ) ) >> (abs(delta_array[j])) );
        subRule.mask = ( (( (oldRuleList[i].mask) & (uint64_t(1) << j) ) ) >> (abs(delta_array[j])) );
      }
      else if (delta_array[j] == 0) {
        // if it is "0", no change
        subRule.value = (( (oldRuleList[i].value) & (uint64_t(1) << j) ) );
        subRule.mask = (( (oldRuleList[i].mask) & (uint64_t(1) << j) ) );
      }
      newRule.value |= subRule.value;
      newRule.mask |= subRule.mask;
      newRule.priority = oldRuleList[i].priority;
      newRule.action = oldRuleList[i].action;
    }
    sumRulesTable.push_back(newRule);
  }
  return sumRulesTable;
}

/*
 * Rearrange each key in the keyTable according to the delta vector
 * because the rules are being reordered by the delta vector
*/

uint64_t keys_rearrange(uint64_t key, vector<int> delta_array)
{
  // Calculate the key rearrangement configure time basing on the delta vector

  uint64_t newKey = 0; // new key after reordering
  for (uint64_t j = 0; j < 64; j++) {
    uint64_t subKey = 0; // subKey is the single bit value of each key
    if (delta_array[j] < 0) {
      subKey = ( (( key & (uint64_t(1) << j) ) ) << (abs(delta_array[j])) );
    }
    else if (delta_array[j] > 0) {
      // if it is positive, do the right shift
      subKey = ( (( key & (uint64_t(1) << j) ) ) >> (abs(delta_array[j])) );
    }
    else if (delta_array[j] == 0) {
      // if it is "0", no change
      subKey = (( key & (uint64_t(1) << j) ) );
    }
    newKey |= subKey;
  }
  return newKey;

}

vector<Rule> tot_rules(int k, vector< vector<Rule> >& bigArray)
{
  vector<Rule> total_rules;
  uint64_t number;
  vector< vector<int> > delta_vector;
  // Allocate an array to hold my class objects
  vector<Trie> tries(k); // Initialize each trie
  // Start to construct the trie data structure here
  for (int j = 0; j < k; j++) {
    // Initilize a set of tries, Each group is a seperate trie
    vector<int> delta_need = generate_delta(bigArray[j]);
    // Push each delta vector into the 2D vector
    delta_vector.push_back(delta_need);
    vector<Rule> newnewTable = rules_rearrange(bigArray[j], delta_need);
    vector<Rule> expand_rules;
    // Doing the rule insertion
    for (int k = 0; k < newnewTable.size(); k++) {
      if ( is_prefix(newnewTable.at(k)) ) {
        number++;
        total_rules.push_back(newnewTable.at(k));
      }
      else {
        int wnum = tries[j].get_new_num( newnewTable.at (k));
        number = number + pow(2, wnum);

      }
    }

  }

  cout << "The size of total_rules with expansion:" << " " << number << endl;

  total_rules.clear();
  return total_rules;
}



/* Caculate the memory cost according to the input rule data
 * input:the group info, after expansion
 * output: total number of trie node = memory cost
 * sets are containers that store unique elements following a specific order
 */
uint64_t real_trienode(int g_num, vector< vector<Rule> >& bigArray)
{
  // a vector of sets: 64 sets for storing the partial rule data

  uint64_t sum = g_num;
  vector<uint64_t> sub_sum(g_num);
  int m;
  vector<Trie> trys(g_num);
  for (m = 0; m < g_num; m++) {
    vector<set<uint64_t>> mysets(64); // Just for value part, unique value
    cout << "The number of rules in Group " << m << " = " << bigArray[m].size() << endl;
    vector<int> delta_need = generate_delta(bigArray[m]);
    // Push each delta vector into the 2D vector
    vector<Rule> newnewTable = rules_rearrange(bigArray[m], delta_need);
    //    cout << "check table size ===== " << newnewTable.size() << endl;
    for (int kk = 0; kk < newnewTable.size(); kk++) {
      if ( is_prefix(newnewTable.at(kk)) ) {
        // if it is prefix rule
        continue;
      }
      else {
        trys[m].expand_rule(newnewTable.at(kk));
        newnewTable.erase(newnewTable.begin() + kk);
        newnewTable.insert(newnewTable.begin() + kk-1,
                           trys[m].expandedTable.begin(), trys[m].expandedTable.end());
        kk = kk -1 + trys[m].expandedTable.size();
      }
    }
    // All the rules are prefix

    for (int q = 0; q < 64; q++) {
      for (int nn = 0; nn < newnewTable.size(); nn++) {
        // Get the number of wildcard at the end
        int w_num = mylog2(newnewTable[nn].mask+1);
        int prefix_len = 64 - w_num; // The length will be inserted into trie
        if ( prefix_len <= q ) {
          // 11**, the prefix_len = 2. if the q = 2, the rule is long enough to calculate the q bit
          newnewTable.erase(newnewTable.begin() + nn);
          nn = nn -1;
        }
        if (prefix_len == q+1) {
          /* Consider about the priority feature
           * 1 0 * * prefix_len = 2, if q = 1, meet this condition
           * 1 * 0 1 need to check whether the next rules were covered by 1 0 * *
           * if it dose, don't need to count either
           */
          for (int nnn = nn+1; nnn < newnewTable.size(); nnn++) {
            if ( (newnewTable[nnn].value >> (64-prefix_len)) ==
                 (newnewTable[nn].value >> (64-prefix_len)) &&
                 newnewTable[nnn].priority >= newnewTable[nn].priority) {
              // Delete the lower priority rules
              newnewTable.erase(newnewTable.begin() + nnn);
              nnn = nnn -1;
            }
          }
        }
      }
      // all qualified rules, calculate the sum
      for (int h = 0; h < newnewTable.size(); h++) {
        /* set[0] is the 63-bit position value
         * set[1] is the 62-bit position
         * set[63] is the 0-bit position
         * we need to calculate the value possibles on every bit position
         * then sum them up
         */
        uint64_t medium = newnewTable[h].value >> (63-q);
        // set[] data structure, only store the unique value. 1, 1, only insert 1 one time
        mysets[q].insert(medium);
      }
      sub_sum[m] += mysets[q].size();
    }

    sum += sub_sum[m];
    //    cout << "group index = " << m <<  " ======check each group's memory size----" << sub_sum[m] << endl;

  }

  cout << "The total size of trie node: " << sum << endl;

  for (int i = 0; i < g_num; i++) {
    trys[i].delete_trie(trys[i].root);
  }
  return sum;
}


//      if (prefix_len == q+1) {
//        /* Consider about the priority feature
//           * 1 0 * * prefix_len = 2, if q = 1, meet this condition
//           * 1 * 0 1 need to check whether the next rules were covered by 1 0 * *
//           * if it dose, don't need to count either
//           */
//        for (int nnn = nn+1; nnn < oneGroup.size(); nnn++) {
//          if ( (oneGroup[nnn].value >> (64-prefix_len)) ==
//               (oneGroup[nn].value >> (64-prefix_len)) &&
//               oneGroup[nnn].priority >= oneGroup[nn].priority) {
//            // Delete the lower priority rules
//            oneGroup.erase(newnewTable.begin() + nnn);
//            nnn = nnn -1;
//          }
//        }
//      }



/* Caculate the prefix len for each rule in a group
 * the rule is without expansion yet
 * rule: * 0 * 1
 * rule: 0 0 1 *
 * output: vector of prefix length for each rule
 */
vector<int> prefixLen_vector(vector<Rule>& group)
{
  vector<int> pre;
  for (int i = 0; i < group.size(); i++) {
    for (int j = 0; j < 64; j++) {
      // Find the first bit "0" from the least significant bit
      // 10x1xx, so the mask is 001011
      if ( ((group[i].mask >> j) & uint64_t(1)) == 0 ) {
        pre.push_back(64 - j);
        break;
      }
    }
  }

  return pre;
}

/* Caculate the memory cost of a cluster
 * if the memory cost MC > threshold, then split the cluster
 * input: one group info, without expansion
 * rule: * 0 * 1
 * rule: 0 0 1 *
 * output: total number of trie node = memory cost
 * sets are containers that store unique elements following a specific order
 */
//uint64_t memory_check(vector<Rule>& group, vector<int> preVector)
//{
//  // a vector of sets: 64 sets for storing the partial rule data
//  vector<int> delta_need = generate_delta(group);
//  vector<Rule> oneGroup = rules_rearrange(group, delta_need);
//  // After swap operation, get the new rule table
//  uint64_t sum = 1; // This is the root node
//  for (int q = 0; q < 64; q++) {
//    for (int nn = 0; nn < oneGroup.size(); nn++) {
//      // Get the number of wildcard at the end
//      if ( preVector[nn] <= q ) {
//        // 11**, the prefix_len = 2. if the q = 2, the rule is long enough to calculate the q bit
//        oneGroup.erase(oneGroup.begin() + nn);
//        preVector.erase(preVector.begin() + nn);
//        nn = nn -1;
//      }
//    }
//    // all qualified rules, calculate the sum
//    for (int h = 0; h < oneGroup.size(); h++) {
//      /* set[0] is the 63-bit position value
//         * set[1] is the 62-bit position
//         * set[63] is the 0-bit position
//         * we need to calculate the value possibles on every bit position
//         * then sum them up
//         */
//      uint64_t medium = oneGroup[h].value >> (63-q);
//      // set[] data structure, only store the unique value. 1, 1, only insert 1 one time
//      mysets[q].insert(medium);
//    }
//    sum += mysets[q].size();
//  }

//  cout << "The total size of trie node in one cluster: " << sum << endl;

//  return sum;
//}

Return_vec kmeans_build_one_trie(int num, vector< vector<Rule> >& bigArray)
{
  Return_vec result;
  //int test_flag = 0; // This variable is used to break the nested loop
  int expandRule_num = 0;
  uint64_t sum_trie_expand_count = 0;
  uint64_t sum_trie_count = 0;
  uint64_t sum_trie_node_count = 0;
  float sum_rule_rearrange_time = 0;

  // Define a 2D vector for storing delta vector
  vector< vector<int> > delta_vector;
  // Allocate an array to hold my class objects
  vector<Trie> tries(num); // Initialize each trie
  // Start to construct the trie data structure here
  for (int j = 0; j < num; j++) {
    vector< pair <uint64_t, uint64_t> > apair = pair_function(bigArray[j]);
    vector<int> delta_need = my_pairsort(apair);
    //        auto start6 = get_time::now();
    clock_t t;
    t = clock();
    vector<Rule> newnewTable = rules_rearrange(bigArray[j], delta_need);

//    printf("-------change the bit order for ruleTable--------\n");
    for (int i = 0; i < newnewTable.size(); i++) {
//      cout << newnewTable[i].value << ", " << newnewTable[i].mask << endl;
      outputFile << newnewTable[i].value << " " << newnewTable[i].mask << endl;
    }



    t = clock() - t;
    sum_rule_rearrange_time += ((float)t)/CLOCKS_PER_SEC;
    //        auto end6 = get_time::now();
    //        auto diff6 = end6 - start6;
    //        sum_rule_rearrange_time += chrono::duration<float>(diff6).count();
    delta_vector.push_back(delta_need);
    // Doing the rule insertion
    for (int k = 0; k < newnewTable.size(); k++) {
      if ( is_prefix(newnewTable.at(k)) ) {
        tries[j].insert_prefix_rule_priority(newnewTable.at(k));
        result.prerules.push_back(bigArray[j][k]);
        result.prefix_index.push_back(k+1);
      }
      else {
        printf("-------insert non-prefix rules-----\n");
        result.expandrules.push_back(bigArray[j][k]);
        result.expand_index.push_back(k+1);
        if ( tries[j].get_new_num( newnewTable.at (k))  > 5 ) {
          cout << "num: Expand too much memory" << endl;
          result.exceed_flag = 1;
          result.tries = tries;
          result.delta = delta_vector;
          result.size_trienode = sum_trie_node_count;
          result.expand_count = sum_trie_expand_count;
          result.rule_conf_time = sum_rule_rearrange_time;
          result.insert_num_rule = sum_trie_count;
          return result;
        }
        tries[j].expand_rule(newnewTable.at(k));
        expandRule_num ++;
      }
    }
    cout << "trie " << j << "'s tire_node is: " << tries[j].node_count << endl;
    sum_trie_expand_count += tries[j].expand_count;
    sum_trie_count += tries[j].count;
    sum_trie_node_count += tries[j].node_count;
  }
  result.tries = tries;
  result.delta = delta_vector;
  result.size_trienode = sum_trie_node_count;
  result.expand_count = sum_trie_expand_count;
  result.rule_conf_time = sum_rule_rearrange_time;
  result.insert_num_rule = sum_trie_count;

  cout << "trie size: " << result.tries.size() << endl;

  cout << "Total insert trie_node count is: " << result.size_trienode << endl;
  cout << "Total insert num of rules is: " << result.insert_num_rule << endl;
  cout << "The num of expanded rules is: " << result.expand_count<< endl;
  cout << "Total rule rearrange time is: " << result.rule_conf_time << endl;
  cout << "Insert time: " << insert_time << endl;
  cout << "Total rule num being expanded is: " << expandRule_num << endl;

  return result;
}


Return_vec kmeans_build_tries(int num, vector< vector<Rule> >& bigArray)
{
  Return_vec result;
  //int test_flag = 0; // This variable is used to break the nested loop
  int expandRule_num = 0;
  uint64_t sum_trie_expand_count = 0;
  uint64_t sum_trie_count = 0;
  uint64_t sum_trie_node_count = 0;
  float sum_rule_rearrange_time = 0;

  // Define a 2D vector for storing delta vector
  vector< vector<int> > delta_vector;
  // Allocate an array to hold my class objects
  vector<Trie> tries(num); // Initialize each trie
  // Start to construct the trie data structure here
  for (int j = 0; j < num; j++) {
    vector< pair <uint64_t, uint64_t> > apair = pair_function(bigArray[j]);
    vector<int> delta_need = my_pairsort(apair);
    //        auto start6 = get_time::now();
    clock_t t;
    t = clock();
    vector<Rule> newnewTable = rules_rearrange(bigArray[j], delta_need);
    t = clock() - t;
    sum_rule_rearrange_time += ((float)t)/CLOCKS_PER_SEC;
    //        auto end6 = get_time::now();
    //        auto diff6 = end6 - start6;
    //        sum_rule_rearrange_time += chrono::duration<float>(diff6).count();
    delta_vector.push_back(delta_need);
    // Doing the rule insertion
    for (int k = 0; k < newnewTable.size(); k++) {
      if ( is_prefix(newnewTable.at(k)) ) {
        tries[j].insert_prefix_rule_priority(newnewTable.at(k));
        result.prerules.push_back(bigArray[j][k]);
        result.prefix_index.push_back(k+1);
      }
      else {
        result.expandrules.push_back(bigArray[j][k]);
        result.expand_index.push_back(k+1);
        if ( tries[j].get_new_num( newnewTable.at (k))  > 5 ) {
          cout << "num: Expand too much memory" << endl;
          //          result.flag = true;
          // remove this kth rule into a new group, recalculate the group
          if (num != 1){
            vector<Rule> new_group;
            new_group.push_back(bigArray[j][k]);
            bigArray[j].erase(bigArray[j].begin() + k);
            bigArray.push_back(new_group);
            // clean the current unuseable tries..
            // need to build more trie
            for (int i = 0; i < num; i++) {
              tries[i].delete_trie(tries[i].root);
            }

            return kmeans_build_tries(bigArray.size(), bigArray);

          }
        }
        tries[j].expand_rule(newnewTable.at(k));
        expandRule_num ++;
      }
    }
    cout << "trie " << j << "'s tire_node is: " << tries[j].node_count << endl;
    sum_trie_expand_count += tries[j].expand_count;
    sum_trie_count += tries[j].count;
    sum_trie_node_count += tries[j].node_count;
  }
  result.tries = tries;
  result.delta = delta_vector;
  result.size_trienode = sum_trie_node_count;
  result.expand_count = sum_trie_expand_count;
  result.rule_conf_time = sum_rule_rearrange_time;
  result.insert_num_rule = sum_trie_count;

  cout << "trie size: " << result.tries.size() << endl;

  cout << "Total insert trie_node count is: " << result.size_trienode << endl;
  cout << "Total insert num of rules is: " << result.insert_num_rule << endl;
  cout << "The num of expanded rules is: " << result.expand_count<< endl;
  cout << "Total rule rearrange time is: " << result.rule_conf_time << endl;
  cout << "Insert time: " << insert_time << endl;
  cout << "Total rule num being expanded is: " << expandRule_num << endl;

  return result;
}


Search_result kmeans_search_result(vector<uint64_t>& keyTable, int k, Return_vec& merged_num_trienode)
{
  Search_result final_result;
  uint64_t actionSum = 0;
  uint64_t checksum = 0; // show the sum of matching priority
  uint64_t match = 0; // how many keys are being matched in these new rules
  //  auto newsum_key_rearrange_time = 0;
  float newsum_key_rearrange_time = 0;
  float decision_time = 0;
  float sTime = 0;
  for (int i = 0; i < keyTable.size(); i++) {
    // Check each key
    vector<uint64_t> matchVector;
    vector<uint32_t> decisionVector;
    for (int m = 0; m < k; m++) {
      clock_t t;
      t = clock();
      //      auto start3 = get_time::now();
      uint64_t newGenKey = keys_rearrange(keyTable[i], merged_num_trienode.delta[m]);
      t = clock() - t;
      newsum_key_rearrange_time += ((float)t)/CLOCKS_PER_SEC;
      //      auto end3 = get_time::now();
      //      auto diff3 = end3 - start3;
      //      newsum_key_rearrange_time += chrono::duration_cast<ns>(diff3).count();
      auto kaishi = get_time::now();
      trie_result search_ret = merged_num_trienode.tries[m].LPM1_search_rule(newGenKey);
      auto jieshu = get_time::now();
      auto qubie = jieshu - kaishi;
      sTime += chrono::duration_cast<ms>(qubie).count();
      attempt += search_ret.cishu;
      // Insert all the priority value, including match and no_match
      matchVector.push_back(search_ret.priority); // Store the priority value
      decisionVector.push_back(search_ret.action);

    }
    vector<uint64_t> test1; // Store the priority value
    vector<uint32_t> test2; // Store the action value
    for (int v = 0; v < matchVector.size(); v++) {
      if (matchVector[v] > 0) {
        uint64_t test = matchVector[v];
        uint32_t action2 = decisionVector[v];
        test1.push_back(test);
        test2.push_back(action2);
      }
    }
    // Choose the smallest one, which means the highest priority
    if (test1.size() > 0) {
      //      auto start88 = get_time::now();
      clock_t t;
      t = clock();
      uint64_t match_final = *min_element(test1.begin(), test1.end());
      t = clock() - t;
      decision_time += ((float)t)/CLOCKS_PER_SEC;
      //      auto end88 = get_time::now();
      //      auto diff88 = end88 - start88;
      //      decision_time += chrono::duration_cast<ms>(diff88).count();
      checksum += match_final;
      match++;
      vector<uint64_t>::iterator it;
      it = find(test1.begin(), test1.end(),match_final);
      int position1 = distance(test1.begin(), it);
      actionSum += test2.at(position1);
      final_result.vv.push_back(i);
      final_result.matched_priority.push_back(match_final);
    }
  }
  final_result.conf_time = newsum_key_rearrange_time;
  final_result.actionSum = actionSum;
  final_result.checksum = checksum;
  final_result.match = match;
  final_result.decision_time = decision_time;
  final_result.sousuo_time = sTime;

  float ratio = (float)final_result.match / keyTable.size();

  cout << "Checksum: " << final_result.checksum << endl;
  cout << "ActionSum: " << final_result.actionSum << endl;
  cout << "Total matches: " << final_result.match << endl;
  cout << "Match ratio: " << ratio << endl;
  cout << "Attempt: " << attempt << endl;
  cout << "Total keys rearrange time is: " << final_result.conf_time << endl;
  cout << "Search time is:" << search_time << endl;
  cout << "Search time is:" << final_result.sousuo_time << endl;
  cout << "Total decision time is:" << final_result.decision_time << endl;
  cout << "==================================================" << endl;
  for (int i = 0; i < k; i++) {
    merged_num_trienode.tries[i].delete_trie(merged_num_trienode.tries[i].root);
  }

  return final_result;
}


vector <vector <uint64_t> > generate_keyTable (vector<uint64_t>& keyTable, int k, Return_vec& merged_num_trienode)
{
  // Generate all the keyTables, num of group
  vector <vector <uint64_t> > keyTable_arrays;
  clock_t t;
  t = clock();
  for (int i = 0; i < k; i++) {
    vector <uint64_t> newkeyTable;
    for (int j = 0; j < keyTable.size(); j++) {
      uint64_t newGenKey = keys_rearrange(keyTable[j], merged_num_trienode.delta[i]);
      outputFile_keys << newGenKey << endl;
      newkeyTable.push_back(newGenKey);
    }
    keyTable_arrays.push_back(newkeyTable); // push the new keyTable into the vector keyTable
    newkeyTable.clear();
  }
  t = clock() - t;
  float newsum_key_rearrange_time = ((float)t)/CLOCKS_PER_SEC;

  cout << "Key rearrange time is: " << newsum_key_rearrange_time << endl;
  return keyTable_arrays;
}

Search_result search_result_1 (vector<uint64_t>& keyTable, vector <vector <uint64_t> >& t_keyTable, Return_vec& merged_num_trienode)
{
  Search_result final_result;
  uint64_t match = 0; // how many keys are being matched in these new rules
  uint64_t attempt = 0;
  clock_t t;
  t = clock();
auto ss = get_time::now();
  /*
 * this is for using the totalNumberLookups metric from HOT project
 * for comparing the performance between HOT and libmatch project
 * 32-bit
 */
//    size_t totalNumberLookups = 100000000;
//    size_t index = 0;
//    for (size_t i = 0; i < totalNumberLookups; ++i) {
//      //  do search operation: totalNumberLookups
//      index = index < keyTable.size() ? index : 0;
//      trie_result search_ret = merged_num_trienode.tries[0].LPM1_search_rule(t_keyTable[0][index]);
//      ++index;
//      attempt += search_ret.cishu;
//      if (search_ret.priority > 0) {
//        match++;
//      }
//    }

    /*
     * this search process is for normal keyTable size
     * original one: don't use totalNumberLookups
     */

//  for (int j = 0; j < keyTable.size(); j++) {
//    outputFile_keys << keyTable[j] << endl;
//  }

//  cout << "after remove the particular key, the number of keys = " << keyTable.size() << endl;


  for (int i = 0; i < keyTable.size(); i++) {
    // Check each key
    trie_result search_ret = merged_num_trienode.tries[0].LPM1_search_rule(t_keyTable[0][i]);
    attempt += search_ret.cishu;
    //cout << "----lookup index i = " << i << ", match = " << ((search_ret.priority > 0) ? 1 : 0) << endl;
    if (search_ret.priority > 0) {
      //cout << "key index = " << i << ", match = 1" << endl;
      match++;
    }
  }

  auto end = get_time::now();
  auto gap = end - ss;
  double newsum_key_search_time = chrono::duration_cast<ns>(gap).count();

//  t = clock() - t;
//  float newsum_key_search_time = ((float)t)/CLOCKS_PER_SEC;

  final_result.sousuo_time = newsum_key_search_time;
  final_result.match = match;
//  float ratio = (float)final_result.match / keyTable.size();

//  float ratio = (float)final_result.match / totalNumberLookups;  //  for using the HOT comparison metric

  cout << "Total matches: " << final_result.match << endl;
  //cout << "Search time is: " << final_result.sousuo_time << endl;
  printf("Search time is = %f\n", final_result.sousuo_time);
//  cout << "Match ratio: " << ratio << endl;
  cout << "Attempt: " << attempt << endl;
  cout << "==================================================" << endl;

  merged_num_trienode.tries[0].delete_trie(merged_num_trienode.tries[0].root);

  return final_result;
}

Search_result search_result_2 (vector<uint64_t>& keyTable, vector <vector <uint64_t> >& vector_keyTable, int k, Return_vec& merged_num_trienode)
{
  Search_result final_result;
  uint64_t match = 0; // how many keys are being matched in these new rules
  uint64_t attempt = 0;
  clock_t t;
  t = clock();
  for (int i = 0; i < keyTable.size(); i++) {
    // Check each key
    int matchVector[k];
    for (int m = 0; m < k; m++) {
      trie_result search_ret = merged_num_trienode.tries[m].LPM1_search_rule(vector_keyTable[m][i]);
      attempt += search_ret.cishu;
      //      cout << search_ret.cishu << endl;
      // Insert all the priority value, including match and no_match
      matchVector[m] = search_ret.priority; // Store the priority value
    }

    for (int v = 0; v < k; v++) {
      if (matchVector[v] != 0) {
        match++;
        break;
      }
    }

  }
  t = clock() - t;
  float newsum_key_search_time = ((float)t)/CLOCKS_PER_SEC;

  final_result.sousuo_time = newsum_key_search_time;
  final_result.match = match;

  float ratio = (float)final_result.match / keyTable.size();

  cout << "Total matches: " << final_result.match << endl;
  cout << "Search time is:" << final_result.sousuo_time << endl;
  cout << "Match ratio: " << ratio << endl;
  cout << "Attempt: " << attempt << endl;
  cout << "==================================================" << endl;

  for (int i = 0; i < k; i++) {
    merged_num_trienode.tries[i].delete_trie(merged_num_trienode.tries[i].root);
  }

  return final_result;
}




// Get the number of "1" bit in a rule, just mask part
int get_numOFone(uint64_t a)
{
  int count = 0; // show the number of wildcard
  for(int i = 0; i < 64; i++) {
    // if this: get the position whose bit is 1 (have wildcard)
    if((a >> i) & uint64_t(1) == 1) {
      count++;
    }
    else {
      continue;
    }
  }
  return count;
}

/* Get the result of intersection of two rules
 * a, b are mask part--show the wildcard information
 */
int intersection(uint64_t a, uint64_t b)
{
  int count = 0; // show the number of wildcard
  if (a == 0 && b == 0){
    count = 1;
  }
  else if (a == 0 && b!= 0) {
    count = 1;
  }
  else if (a != 0 && b == 0) {
    count = 1;
  }
  else {
    for(int i = 0; i < 64; i++) {
      if ( ((a >> i) & uint64_t(1) == 1) && ((b >> i) & uint64_t(1) == 1) ) {
        count++;
      }
      else {
        continue;
      }
    }
  }

  return count;
}

/* Calculate the similiary matrix
 * the value: [0, 1]
 * fraction
 */
float sim_value(Rule a, Rule b)
{
  float result;
  int count = 0;
  int count1 = 0;
  int count2 = 0;
  if (a.mask != 0 && b.mask != 0) {
    int fenzi = get_numOFone(a.mask & b.mask);
    int fenmu = get_numOFone(a.mask | b.mask);
    result = fenzi / fenmu;
    return result;
  }
  if (a.mask == 0 && b.mask == 0) {
    for(int i = 0; i < 64; i++) {
      if ((a.value >> i) & uint64_t(1) == (b.value >> i) & uint64_t(1)) {
        count++;
      }
    }
    // Get the number of same bit, either 0 or 1
    result = count / 64;
    return result;
  }
  if (a.mask == 0 && b.mask != 0) {
    for(int i = 0; i < 64; i++) {
      if ((b.mask >> i) & uint64_t(1) == 0) {
        // mask = 1 bit number
        count1++;
        if ((a.value >> i) & uint64_t(1) == (b.value >> i) & uint64_t(1)) {
          count2++;
        }
      }
    }
    result = count2 / count1;
    return result;
  }
  if (a.mask != 0 && b.mask == 0) {
    for(int i = 0; i < 64; i++) {
      if ((a.mask >> i) & uint64_t(1) == 0) {
        // mask = 1 bit number
        count1++;
        if ((a.value >> i) & uint64_t(1) == (b.value >> i) & uint64_t(1)) {
          count2++;
        }
      }
    }
    result = count2 / count1;
    return result;
  }
}



int neighbor(uint64_t a, uint64_t b)
{
  int linju = 0;

  for(int i = 0; i < 64; i++) {
    if ( ((a >> i) & uint64_t(1) == 1) && ((b >> i) & uint64_t(1) == 1) ) {
      linju = 1;
      return linju;
    }
  }
  return linju;

  //  int distance = get_distance(a, b);
  //  if (distance == 0) {
  //    linju = 1;
  //  }
  //  else {
  //    linju = 0;
  //  }
  //  return linju;
}



/* Check the memory cost for each cluster
 * used for the assign_cluster()
 * to choose the cluster when the mask's distance is the same
 * to compare the value's distance: memory cost
 * we choose the least memory cost cluster
 */
int real_trienode_group(int g_num, vector< vector<Rule> >& bigArray)
{
  uint64_t sum = 0;
  vector<uint64_t> sub_sum(g_num);
  vector<Trie> trys(g_num);
  for (int m = 0; m < g_num; m++) {
    vector<set<uint64_t>> mysets(64); // Just for value part, unique value
    vector<int> delta_need = generate_delta(bigArray[m]);
    // Push each delta vector into the 2D vector
    vector<Rule> newnewTable = rules_rearrange(bigArray[m], delta_need);
    for (int k = 0; k < newnewTable.size(); k++) {
      if ( is_prefix(newnewTable.at(k)) ) {
        continue;
      }
      else {
        cout << "group index = " << m << ", rule index = " << k << endl;
        trys[m].expand_rule(newnewTable.at(k));
        newnewTable.erase(newnewTable.begin() + k);
        newnewTable.insert(newnewTable.begin() + k,
                           trys[m].expandedTable.begin(), trys[m].expandedTable.end());
        k = k -1 + trys[m].expandedTable.size();
      }
    }

    for (int q = 0; q < 64; q++) {
      for (int nn = 0; nn < newnewTable.size(); nn++) {
        int w_num = mylog2(newnewTable[nn].mask+1);
        int prefix_len = 64 - w_num;
        if ( prefix_len <= q ) {
          // This rule is not a qualifier, need to remove
          newnewTable.erase(newnewTable.begin() + nn);
          nn = nn -1;
        }
        if (prefix_len == q+1) {
          // Consider about the priority feature
          for (int nnn = nn+1; nnn < newnewTable.size(); nnn++) {
            if ( (newnewTable[nnn].value >> (64-prefix_len)) ==
                 (newnewTable[nn].value >> (64-prefix_len)) &&
                 newnewTable[nnn].priority >= newnewTable[nn].priority) {
              // Delete the lower priority rules
              newnewTable.erase(newnewTable.begin() + nnn);
              nnn = nnn -1;
            }
          }
        }
      }
      // all qualified rules, calculate the sum
      for (int h = 0; h < newnewTable.size(); h++) {
        uint64_t medium = newnewTable[h].value >> (63-q);
        mysets[q].insert(medium);
      }
      sub_sum[m] += mysets[q].size();
    }

    sum += sub_sum[m];
  }

  uint64_t min_trieNode_group = *min_element(sub_sum.begin(), sub_sum.end());
  vector<uint64_t>::iterator it;
  it = find(sub_sum.begin(), sub_sum.end(), min_trieNode_group);
  int choose_cluster_index = distance(sub_sum.begin(), it);

  return choose_cluster_index;
}


/* Check the memory cost crash in a cluster
 * if return false, crash
 * if return true, ok
 */
bool check_MC(vector<Rule> & bigArray)
{
  Trie trys;
  vector<int> delta_need = generate_delta(bigArray);
  // Push each delta vector into the 2D vector
  vector<Rule> newnewTable = rules_rearrange(bigArray, delta_need);
  for (int k = 0; k < newnewTable.size(); k++) {
    if ( !is_prefix(newnewTable.at(k)) ) {
      if ( trys.get_new_num( newnewTable.at (k))  > 15 ) {
        return false;
      }
    }
  }
  return true;
}


/* Evaluate it is neighbor of the vertices
 * how many "1" are at the same bit positions.
 * input: the vertices/center of a cluster and the left points
 * output: if ret > 0 is a neighbor
 */
int sizeof_same(uint64_t a, uint64_t b)
{
  int ret = intersection(a, b);

  return ret;
}

/* Return the correlation matrix among all the points
 * using sizeof_same() function
 */
vector< vector<int> > get_cmatrix(vector<Rule> points)
{
  vector< vector<int> > cmatrix(points.size(), vector<int>(points.size()));
  for (int i = 0; i < points.size(); i++) {
    for (int j = 0; j < points.size(); j++) {
      if (i == j) {
        cmatrix[i][j] = 64;
      }
      else {
        cmatrix[i][j] = sizeof_same(points[i].mask, points[j].mask);
      }
      //      cmatrix[i][j] = neighbor(points[i].mask, points[j].mask);
    }
  }

  return cmatrix;
}





int get_center_index(vector< vector<int> > matrix)
{
  vector<int> sum_each_row(matrix.size());
  for (int i = 0; i < matrix.size(); i++) {
    for (int j = 0; j < matrix.size(); j++) {
      sum_each_row[i] += matrix[i][j];
    }
  }
  int max_neighbor = *max_element(sum_each_row.begin(), sum_each_row.end());
  vector<int>::iterator it;
  it = find(sum_each_row.begin(), sum_each_row.end(),max_neighbor);
  int index = distance(sum_each_row.begin(), it);

  return index;
}



/* return rst
 * rst[0] is cluster
 * rst[1] is the non-neighbor vector
 */
vector <vector<Rule> > cluster_CC(vector<Rule> points, vector<int> row)
{
  vector <vector<Rule> > rst(2);
  for (int j = 0; j < points.size(); j++) {
    if (row[j] > 0) {
      // row[j] = 0: there is no intersection between a.mask and b.mask
      // a.mask > 0 and b.mask > 0
      // means j is a neighbor of ith row
      rst[0].push_back(points[j]);
    }
    else {
      rst[1].push_back(points[j]);
    }
  }

  return rst;
}



/* Correlation Clustering Method
 * Input: all the points
 * using similiarity function
 */
void assign_cluster_CC(vector< vector<Rule> > assigned_clusters, vector<Rule> points, vector< vector<int> > matrix)
{
  // Choose the maximal similiarity point
  int index_row = get_center_index(matrix);
  vector <vector<Rule> > rr = cluster_CC(points, matrix[index_row]);
  assigned_clusters.push_back(rr[0]);
  if (rr[1].size() == 0) {
    target = assigned_clusters;
    return;
  }
  else {
    points = rr[1];
    matrix = get_cmatrix(rr[1]);
    assign_cluster_CC(assigned_clusters, points, matrix);
  }
}




int get_num_group(vector <vector<Rule>> final_cluster)
{
  int num_group = 0;
  for (int i = 0; i < final_cluster.size(); i++) {
    if (final_cluster[i].size() > 0) {
      num_group++;
    }
    else {
      return num_group;
    }

  }
  return num_group;
}

vector<uint64_t> remove_duplicate(vector<uint64_t> keyTable) {
  vector<uint64_t> nond_keytable;
  nond_keytable.reserve(keyTable.size());
  set<uint64_t> duplicateChecker;
  int numberDuplicates = 0;
  for (size_t i = 0; i < keyTable.size(); ++i) {
    uint64_t currentValue = 0;
    currentValue = keyTable[i];
    if(duplicateChecker.find(currentValue) != duplicateChecker.end()) {
      // find a duplicate in the keyTable
      ++numberDuplicates;
    } else {
      duplicateChecker.insert(currentValue);
      nond_keytable.push_back(currentValue);
    }
  }
  cout << "Size of non-duplicate keyTable = " << nond_keytable.size() << endl;
  return nond_keytable;

}

//vector<Rule> remove_duplicate_rule(vector<Rule> ruleTable) {
//  vector<Rule> nond_ruleTable;
//  nond_ruleTable.reserve(ruleTable.size());
//  set<uint64_t> duplicateChecker;
//  int numberDuplicates = 0;
//  for (size_t i = 0; i < ruleTable.size(); ++i) {
//    uint64_t currentValue = 0;
//    currentValue = ruleTable[i].value;
////    cout << "i = " << i << "," << currentValue << endl;
//    if(duplicateChecker.find(currentValue) != duplicateChecker.end()) {
//      // find a duplicate in the ruleTable
//      ++numberDuplicates;
//    } else {
//      duplicateChecker.insert(currentValue);
//      nond_ruleTable.push_back(ruleTable[i]);
//    }
//  }
////  cout << "Size of duplicateChecker = " << duplicateChecker.size() << endl;
////  cout << "Size of non-duplicate ruleTable = " << nond_ruleTable.size() << endl;
//  return nond_ruleTable;

//}

vector<Rule> remove_duplicate_rule(vector<Rule> ruleTable) {
  vector<Rule> nond_ruleTable;
  nond_ruleTable.reserve(ruleTable.size());
  set<std::pair<uint64_t, uint64_t>> duplicateChecker;
  int numberDuplicates = 0;
  for (size_t i = 0; i < ruleTable.size(); ++i) {
    std::pair<uint64_t, uint64_t> currentPair;
    currentPair.first = ruleTable[i].value;
    currentPair.second = ruleTable[i].mask;

    if(duplicateChecker.find(currentPair) != duplicateChecker.end()) {
      // find a duplicate in the ruleTable
      ++numberDuplicates;
    } else {
      duplicateChecker.insert(currentPair);
      nond_ruleTable.push_back(ruleTable[i]);
    }
  }

  return nond_ruleTable;

}

inline bool mySortFunction (Rule pair1, Rule pair2)
{
  return (pair1.mask < pair2.mask);
}


int main(int argc, char* argv[])
{
  // Set the hard line for the memory cost == number of trie node
  threshold = stoull(argv[4]);
  //  delta = stoull(argv[5]);
  //  cluster_num = stoull(argv[6]);
  ifstream file (argv[1]);
  // Read the rules from txt file
  vector<Rule> oldpingRulesTable;
  int i = 0;
  if (file.is_open()) {
    while (!file.eof()) {
      // Read lines as long as the file is
      string line;
      getline(file,line);
      if(!line.empty()) {
        Rule rule = strTint(line);
        // Add the priority feature when generating rules
        // Priority is in-order of generating
//        rule.action = actions[i];
        i = i + 1;
        rule.priority = i;
        // Push the input file into ruleArray
        oldpingRulesTable.push_back(rule);
      }
    }
  }
  file.close();

  // Read in keys from file:
  ifstream file1 (argv[2]);
  vector<uint64_t> keyTable;

  if (file1.is_open()) {

    while (!file1.eof()) {
      // Read lines as long as the file is
      string packet;
      getline(file1,packet);

      if(!packet.empty()) {
        uint64_t key = stoull(packet);
        // Push the input file into ruleArray
        keyTable.push_back(key);
      }
    }

  }

  file1.close();


  cout << "######CC algorithm#####" << endl;
  cout << "The num of keys: " << keyTable.size() << endl;
  cout << "The num of rules: " << oldpingRulesTable.size() << endl;

  /*
   * remove the duplicate rules and keys
   */
keyTable = remove_duplicate(keyTable);
oldpingRulesTable = remove_duplicate_rule(oldpingRulesTable);

/*
 * Down below is to get the non-duplicated rule set
 * size = 6841
 */
//ofstream file_nond_rule (argv[6]);
//string line_new;
//for (int i = 0; i < oldpingRulesTable.size(); i++) {
//  line_new = to_string(oldpingRulesTable[i].value);
//  file_nond_rule << line_new << endl;
//}
//file_nond_rule.close();

cout << "non-duplicated keytable size = " << keyTable.size() << ", " << "non-duplicated ruletable size = " << oldpingRulesTable.size() << endl;

  vector<Rule> data(oldpingRulesTable);  // The input rules
  vector< vector <Rule> > one_cluster;

  /*
   * sort the vector:data, by the Rule.mask, mask values
   */

  std::sort(data.begin(), data.end(), mySortFunction);

  #ifdef PING_DEBUG3
  for (uint i = 0; i < data.size(); i++) {
    std::cout << data[i].value << ", " << data[i].mask << std::endl;
  }
#endif


  one_cluster.push_back(data);

  Return_vec mm_cost = kmeans_build_one_trie(1, one_cluster);
  // even for one cluster, the keytable still needs to be transformed

  if (mm_cost.exceed_flag == 0) {
    cout << "The num of clusters is: " << 1 << endl;
    vector< vector <uint64_t> > t_keytable = generate_keyTable(keyTable, one_cluster.size(), mm_cost);
    search_result_1(keyTable, t_keytable, mm_cost);
    cout << "DEBUG TEST: " << debug << endl;
    return 0;
  }
  else {
    mm_cost.tries[0].delete_trie(mm_cost.tries[0].root);
  }
  //  mm_cost.tries[0].delete_trie(mm_cost.tries[0].root);

  vector< vector<int> > rst_matrix = get_cmatrix(data);
  vector< vector <Rule> > clusters;

  clock_t t;  // calculating CPU time, which is smaller than the Wall time
  t = clock();
  assign_cluster_CC(clusters, data, rst_matrix);
  t = clock() - t;
  cout << "The clustering time cost is: " << ((float)t)/CLOCKS_PER_SEC << endl;

  cout << "==========================" << endl;
  cout << "The num of clusters is: " << target.size() << endl;

  Return_vec trienode = kmeans_build_tries(target.size(), target);
  //  uint64_t num_trienode = real_trienode(target.size(), target);
  //  cout << "TEST trie node num: " << num_trienode << endl;
  //  kmeans_search_result(keyTable, target.size(), trienode);
//  cout << "number of tries: " << trienode.tries.size() << endl;
//  cout << "target.size = " << target.size() << endl;
  vector< vector <uint64_t> > testping = generate_keyTable(keyTable, trienode.tries.size(), trienode);

  search_result_2(keyTable, testping, trienode.tries.size(), trienode);


  cout << "DEBUG TEST: " << debug << endl;

  return 0;
}






