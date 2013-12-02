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

set_FILTERED_OUTPUT() {
    local URL="$1"

    local LOG_LINE="CACHE_MACHINE"
    LOG_LINE="$LOG_LINE	SEQUENCE_NUMBER"
    LOG_LINE="$LOG_LINE	TIMESTAMP"
    LOG_LINE="$LOG_LINE	DURATION"
    LOG_LINE="$LOG_LINE	IP"
    LOG_LINE="$LOG_LINE	STATUS_CODE"
    LOG_LINE="$LOG_LINE	SIZE"
    LOG_LINE="$LOG_LINE	REQUEST_METHOD"
    LOG_LINE="$LOG_LINE	$URL"
    LOG_LINE="$LOG_LINE	HIERARCHY_STATUS_OR_PEER_IP"
    LOG_LINE="$LOG_LINE	MIME"
    LOG_LINE="$LOG_LINE	REFERRER"
    LOG_LINE="$LOG_LINE	X_FORWARDED_FOR	USER_AGENT"
    LOG_LINE="$LOG_LINE	ACCEPT_LANGUAGE"
    LOG_LINE="$LOG_LINE	X_ANALYTICS"

    FILTERED_OUTPUT="$("$FILTER" <<<"$LOG_LINE" 2>&1 || echo "Filter bailed out")"
}

assert_counted() {
    local URL="$1"
    local COUNTED_PROJECT="$2"
    local COUNTED_PAGE="$3"

    shift 3

    start_test "Assure to count $URL"

    local FILTERED_OUTPUT=
    set_FILTERED_OUTPUT "$URL" "$@"

    if [ "$FILTERED_OUTPUT" = "$COUNTED_PROJECT 1 SIZE $COUNTED_PAGE" ]
    then
	mark_test_passed
    else
	mark_test_failed
    fi
}

assert_not_counted() {
    local URL="$1"

    shift 1

    start_test "Assure to not count $URL"

    local FILTERED_OUTPUT=
    set_FILTERED_OUTPUT "$URL" "$@"

    if [ -z "$FILTERED_OUTPUT" ]
    then
	mark_test_passed
    else
	mark_test_failed
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

# Single log line tests --------------------------------------------------------
assert_not_counted "http://upload.wikimedia.org/wikipedia/commons/f/f4/Ambox_content.png"
assert_not_counted "http://en.wikipedia.org/Not_Countworthy_(No_Slash_Wiki_Slash)"
assert_counted     "http://en.wikipedia.org/wiki/Countworthy_on_enwiki" "en" "Countworthy_on_enwiki"
assert_counted     "http://fr.wikipedia.org/wiki/Countworthy_on_frwiki" "fr" "Countworthy_on_frwiki"
assert_counted     "http://en.wikipedia.org/wiki/Special:Some_Countworthy_Special_Page" "en" "Special:Some_Countworthy_Special_Page"
assert_not_counted "http://en.wikipedia.org/wiki/Special:CentralAutoLogin/createSession?gu_id=0&type=script&proto=http"
assert_not_counted "http://en.wikipedia.org/wiki/Special:CentralAutoLogin/checkLoggedIn?wikiid=commonswiki&proto=http&type=script"
assert_counted     "http://en.wikipedia.org/wiki/Special:CentralAutoLogin" "en" "Special:CentralAutoLogin"
assert_counted     "http://en.wikipedia.org/wiki/Special:CentralAutoLoginX/Countworthy" "en" "Special:CentralAutoLoginX/Countworthy"

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
