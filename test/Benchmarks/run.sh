#!/bin/bash

echo "=====hot vs lib====="


./../../build/test/hot_tree.test -insert=file -input=4096rules_hot.txt -size=2742 -lookup=file -lookupFile=hot_small_keys_trans_1.txt

./../../build/test/prefix_kmeans.test ffff_lib_rule.txt ffff_lib_key.txt action8192.txt 26666670 1 2

./../../build/test/hot_tree.test -insert=file -input=hot_rule5620.txt -size=3892 -lookup=file -lookupFile=hot_rule5620_small_keys_trans.txt


./../../build/test/prefix_kmeans.test lib_rule5620.txt lib_rule5620_small_keys_trans.txt action8192.txt 26666670 1 2







echo "=====finish====="




