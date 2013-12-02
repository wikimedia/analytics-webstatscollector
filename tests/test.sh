#!/bin/bash
FILTER="../filter"

TESTS=0
TESTS_GOOD=0
FIRST_FAILED_TEST_NAME=

start_test() {
    local NAME="$1"

    TESTS=$((TESTS+1))
    CURRENT_TEST_NAME="$NAME"

    echo -n "Test #${TESTS}: ${CURRENT_TEST_NAME} ... "
}

mark_test_passed() {
    TESTS_GOOD=$((TESTS_GOOD+1))
    echo "pass"
}

mark_test_failed() {
    echo "FAIL!"
    if [ -z "$FIRST_FAILED_TEST_NAME" ]
    then
	FIRST_FAILED_TEST_NAME="$CURRENT_TEST_NAME"
	FIRST_FAILED_TEST_NUMBER="$TESTS"
    fi
}

start_test "Spaceless lines in filter" #----------------------------------------
TEST_SPACELESS_LINES=`cat entries-with-urls-with-spaces-2013-02-10.txt | "$FILTER" | perl -MData::Dumper -ne '@f=split(/\s/,$_,4);  print if $f[3] =~ /\ /;' | wc -l`

if [ $TEST_SPACELESS_LINES -eq 0 ]; then
  mark_test_passed
else
  mark_test_failed
  exit -1;
fi


start_test "Big fields in filter" #---------------------------------------------
trap "" SIGSEGV
cat big-entry-1.txt | "$FILTER" >/dev/null 2>/dev/null

if [ $? -ne 139 ]; then
  mark_test_passed
else
  mark_test_failed
fi

start_test "in->project is empty" #---------------------------------------------
TEST_PROJECT_EMPTY=`cat entry-line1.txt | "$FILTER" | wc -l`;

if [ $? -ne 139 -a $TEST_PROJECT_EMPTY -eq 0 ]; then
  mark_test_passed
else
  mark_test_failed
fi

# -- printing statistics -------------------------------------------------------

TESTS_FAILED=$((TESTS-TESTS_GOOD))

if [ $TESTS_FAILED -eq 0 ]
then
    TESTS_FAILED_APPENDIX=
else
    TESTS_FAILED_APPENDIX=" (first failed test is #${FIRST_FAILED_TEST_NUMBER}: '$FIRST_FAILED_TEST_NAME')"
fi

cat <<EOF

Test statistics:
  # of tests        : ${TESTS}
  # of passed tests : ${TESTS_GOOD}
  # of failed tests : ${TESTS_FAILED}${TESTS_FAILED_APPENDIX}

EOF

exit $TESTS_FAILED
