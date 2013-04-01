#!/bin/bash
TEST_SPACELESS_LINES=`cat entries-with-urls-with-spaces-2013-02-10.txt | ./filter | perl -MData::Dumper -ne '@f=split(/\s/,$_,4);  print if $f[3] =~ /\ /;' | wc -l`


if [ $TEST_SPACELESS_LINES -eq 0 ]; then
  echo "Test1: Spaceless lines in filter PASSED";
else
  echo "Test1: Spaceless lines in filter FAILED";
  exit -1;
fi


trap "" SIGSEGV
cat big-entry-1.txt | ./filter >/dev/null 2>/dev/null

if [ $? -ne 139 ]; then
  echo "Test2: Big fields in filter PASSED";
else
  echo "Test2: Big fields in filter FAILED";
fi


