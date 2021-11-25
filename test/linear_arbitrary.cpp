// Copyright (c) 2015 Flowgrammable.org
// All rights reserved

#include "../libmatch/linear_arbitrary.cpp"


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
#include <time.h>
#include <set>

using namespace std;
using  ns = chrono::nanoseconds;
using  ms = chrono::microseconds;
using get_time = chrono::steady_clock ;


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


  rule.value = stoull(substr1);
  rule.mask = stoull(substr2);
  rule.priority = 0;

  return rule;
}

// Sorting the rules in an asscending order
bool wayToSort(Rule aa, Rule bb)
{
  return (aa.mask < bb.mask);
}

bool wayToSort1(Rule aaa, Rule bbb)
{
  return (aaa.value < bbb.value);
}

/*
 * Sorting the prefix format rules into asscending order, denpending on the prefix length
 * using the std::sort function
*/
vector<Rule> sort_rules(vector<Rule>& ruleList)
{
  std::sort(ruleList.begin(), ruleList.end(), wayToSort);
  /*
  cout << "mark" << "===============" << endl;
  for (int k = 0; k < ruleList.size(); k ++) {
    cout << ruleList[k].value << ", " << ruleList[k].mask << endl;
  }
  */
  vector<Rule> sortTable;
  vector<Rule> sortTotalTable;
  // Determine the size of combined table
  sortTotalTable.reserve(ruleList.size());
  for (uint i = 0; i < ruleList.size(); i ++) {
    if (i != ruleList.size() - 1) {
      if (ruleList.at(i).mask == ruleList.at(i+1).mask) {
        // if the mask value is the same, push into the same vector
        //cout << "test" << endl;
        sortTable.push_back(ruleList.at(i));
        continue;
      }
      else {
        sortTable.push_back(ruleList.at(i));
        //cout << "i = " << i << endl;
        std::sort(sortTable.begin(), sortTable.end(), wayToSort1);
        sortTotalTable.insert( sortTotalTable.end(), sortTable.begin(), sortTable.end() );
        /*
        for (int k = 0; k < sortTotalTable.size(); k ++) {
          cout << sortTotalTable[k].value << ", " << sortTotalTable[k].mask << endl;
        }
        cout << "sortTotalTable size = " << sortTotalTable.size() << endl;
        */
        // Delete the current contents, clear the memory
        sortTable.clear();
        //cout << "sortTable size = " << sortTable.size() << endl;
        continue;
      }
    }
    else {
      // for the last element in the vector
      // for avoiding the over-range of the vector
      if (ruleList.at(i).mask == ruleList.at(i-1).mask) {
        sortTable.push_back(ruleList.at(i));
        //cout << "i = " << i << endl;
        std::sort(sortTable.begin(), sortTable.end(), wayToSort1);
        sortTotalTable.insert( sortTotalTable.end(), sortTable.begin(), sortTable.end() );
      }
      else {
        std::sort(sortTable.begin(), sortTable.end(), wayToSort1);
        sortTotalTable.insert( sortTotalTable.end(), sortTable.begin(), sortTable.end() );
        sortTable.clear();
        sortTable.push_back(ruleList.at(i));
        sortTotalTable.insert( sortTotalTable.end(), sortTable.begin(), sortTable.end() );
      }
    }
  }
  return sortTotalTable;
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



int main(int argc, char* argv[])
{
  // Add the thrid parameter: random action
  vector<int> actions;
  string line1;
  uint32_t action1;
  ifstream file2 (argv[3]);
  // Read action from the txt file
  if (file2.is_open()) {
    while (!file2.eof()) {
      getline(file2, line1);
      if (!line1.empty()) {
        action1 = stoull(line1);
        actions.push_back(action1);
      }
    }
  }
  file2.close();


  string line;
  Rule rule;
  ifstream file (argv[1]);

  // Read in rules from file:
  vector<Rule> oldinputRules;
  int i = 0;
  if (file.is_open()) {
    while (!file.eof()) {
      // Read lines as long as the file is
      getline(file,line);
      if(!line.empty()) {
        rule = strTint(line);
        // Add the priority feature when generating rules
        // Priority is in-order of generating
        rule.action = actions[i];
        i = i + 1;
        rule.priority = i;
        //rule.action = rand() % 50 + 1; // randomly choose from [1,50]
        // Push the input file into ruleArray
        oldinputRules.push_back(rule);
      }
    }
  }
  file.close();


  // Read in keys from file:
  string packet;
  uint64_t key;
  ifstream file1 (argv[2]);
  vector<uint64_t> keyTable;
  if (file1.is_open()) {
    while (!file1.eof()) {
      // Read lines as long as the file is
      getline(file1,packet);
      if(!packet.empty()) {
        key = stoull(packet);
        // Push the input file into ruleArray
        keyTable.push_back(key);
      }
    }
  }
  file1.close();

  cout << "hot keytable size = " << keyTable.size() << endl;

  keyTable = remove_duplicate(keyTable);  //  need to add this function whenever using HOT library for comparisons.
  oldinputRules = remove_duplicate_rule(oldinputRules);
  cout << "non-duplicated keytable size = " << keyTable.size() << ", " << "non-duplicated ruletable size = " << oldinputRules.size() << endl;

  // Insert rules into linear arbitrary table:
  linearTable table;

  clock_t t;  // calculating CPU time, which is smaller than the Wall time
  t = clock();
  for (uint i = 0; i < oldinputRules.size(); i++) {
    table.insert_rule(oldinputRules.at(i));
  }

  t = clock() - t;
  float insert_time = ((float)t)/CLOCKS_PER_SEC;

  // Search the rules

  uint64_t match = 0;


  clock_t t1;  // calculating CPU time, which is smaller than the Wall time
  t1 = clock();

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
//      trie_result decision = table.search_rule(keyTable[index]);
//      ++index;
//      attempt += decision.cishu;
//      if (decision.priority > 0) {
//        match++;
//      }
//    }

  /*
   * this search process is for normal keyTable size
   * original one: don't use totalNumberLookups
   */

  for (size_t i = 0; i < keyTable.size(); ++i) {
    trie_result decision = table.search_rule(keyTable[i]);
    attempt += decision.cishu;
    if (decision.priority > 0) {
      match++;
    }
  }


  t1 = clock() - t1;
  float search_time = ((float)t1)/CLOCKS_PER_SEC;

//  float ratio = (float)match / keyTable.size();

//    float ratio = (float)match / totalNumberLookups;  //  for using the HOT comparison metric

  cout << "The num of keys: " << keyTable.size() << endl;
  cout << "The num of rules: " << oldinputRules.size() << endl;
  cout << "Total matches: " << match << endl;
//  cout << "Match ratio: " << ratio << endl;
  cout << "Attempt: " << attempt << endl;
  cout << "Insertion time is: " << insert_time << endl;
  cout << "Search time is: "<< search_time << endl;
  cout << "==================================================" << endl;

  return 0;
}


