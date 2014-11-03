#!/bin/bash
FILTER="../filter"
COLLECTOR="../collector"
COLLECTOR_PORT="3815" # currently hardcoded in collector.c

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
    local REASON="$1"
    if [ ! -z "$REASON" ]
    then
        REASON=" ($REASON)"
    fi

    echo "FAIL!$REASON"

    if [ -z "$FIRST_FAILED_TEST_NAME" ]
    then
	FIRST_FAILED_TEST_NAME="$CURRENT_TEST_NAME"
	FIRST_FAILED_TEST_NUMBER="$TESTS"
    fi
}

set_FILTERED_OUTPUT() {
    local URL="$1"

    local LOCAL_LOG_LINE_IP="${LOG_LINE_IP:-IP}"
    unset LOG_LINE_IP

    local LOCAL_LOG_LINE_STATUS="${LOG_LINE_STATUS:-STATUS}"
    unset LOG_LINE_STATUS

    local LOG_LINE="CACHE_MACHINE"
    LOG_LINE="$LOG_LINE	SEQUENCE_NUMBER"
    LOG_LINE="$LOG_LINE	TIMESTAMP"
    LOG_LINE="$LOG_LINE	DURATION"
    LOG_LINE="$LOG_LINE	$LOCAL_LOG_LINE_IP"
    LOG_LINE="$LOG_LINE	$LOCAL_LOG_LINE_STATUS"
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

    local EXPECTED_OUTPUT="$COUNTED_PROJECT 1 SIZE $COUNTED_PAGE"
    if [ "$FILTERED_OUTPUT" = "$EXPECTED_OUTPUT" ]
    then
	mark_test_passed
    else
	mark_test_failed "Expected output: '$EXPECTED_OUTPUT'. Actual output: '$FILTERED_OUTPUT'"
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
	mark_test_failed "Expected to be not counted, but counted as '$FILTERED_OUTPUT'"
    fi
}

test_collector() {
    local COLLECTOR_TEST_SPEC_DIR_RELS="$1"
    start_test "$COLLECTOR_TEST_SPEC_DIR_RELS"

    local SCRIPT_DIR_ABS="$(pwd)"
    local COLLECTOR_TEST_SPEC_DIR_ABS="$SCRIPT_DIR_ABS/$COLLECTOR_TEST_SPEC_DIR_RELS"
    local INPUT_FILE_ABS="$COLLECTOR_TEST_SPEC_DIR_ABS/input"
    local EXPECTED_PAGECOUNTS_FILE_ABS="$COLLECTOR_TEST_SPEC_DIR_ABS/expected.pagecounts"
    local EXPECTED_PROJECTCOUNTS_FILE_ABS="$COLLECTOR_TEST_SPEC_DIR_ABS/expected.projectcounts"

    # Checking if the test specification files exist
    if [ ! -e "$INPUT_FILE_ABS" ]
    then
        mark_test_failed "The input file '$INPUT_FILE_ABS' does not exist"
        return
    fi

    if [ ! -e "$EXPECTED_PAGECOUNTS_FILE_ABS" ]
    then
        mark_test_failed "The expected pagecounts file '$EXPECTED_PAGECOUNTS_FILE_ABS' does not exist"
        return
    fi

    if [ ! -e "$EXPECTED_PROJECTCOUNTS_FILE_ABS" ]
    then
        mark_test_failed "The expected projectcounts file '$EXPECTED_PROJECTCOUNTS_FILE_ABS' does not exist"
        return
    fi

    if [ "$(netstat --udp --listening --numeric | sed -e 's/^[^:]*:\([0-9]*\) .*/\1/' | grep -c "^$COLLECTOR_PORT\$")" != "0" ]
    then
        mark_test_failed "It seems another process is already listening on UDP-Port $COLLECTOR_PORT, so I cannot run collector tests"
        return
    fi

    # Setup test directory
    local TEST_TMP_DIR_ABS="$(mktemp --directory --tmpdir webstatscollector-tests.XXXXXX)"
    pushd "$TEST_TMP_DIR_ABS" >/dev/null
    mkdir dumps

    local COLLECTOR_STDOUT_FILE_ABS="$TEST_TMP_DIR_ABS/collector.stdout"
    local COLLECTOR_STDERR_FILE_ABS="$TEST_TMP_DIR_ABS/collector.stderr"

    local FAILURE=""

    # The collector will dump on the full hour. So if we're susciciously close
    # to that, we back off for a bit, to avoid unwarranted dumping.
    if [ $(date +1%M%S) -gt 15955 ]
    then
        sleep 6s
    fi

    # start collector
    "$SCRIPT_DIR_ABS/$COLLECTOR" >"$COLLECTOR_STDOUT_FILE_ABS" 2>"$COLLECTOR_STDERR_FILE_ABS" &
    local COLLECTOR_PID=$!

    local COLLECTOR_FD="/dev/udp/127.0.0.1/$COLLECTOR_PORT"

    # Give some time for collector to start
    sleep 2s

    # Feed the input and force a dump
    cat <"$INPUT_FILE_ABS" >"$COLLECTOR_FD"
    echo -e "\n-produceDump" >"$COLLECTOR_FD"
    sleep 5s

    # Tear down the collector
    kill "$COLLECTOR_PID"
    sleep 0.1s
    if [ -e "/proc/$COLLECTOR_PID" ]
    then
        sleep 0.4s
        if [ -e "/proc/$COLLECTOR_PID" ]
        then
            kill -9 "$COLLECTOR_PID"
            FAILURE="Collector did not exit within 0.5s"
        fi
    fi

    # Checking pagecounts file
    if [ -z "$FAILURE" ]
    then
        if ! diff -Naur dumps/pagecounts-* "$EXPECTED_PAGECOUNTS_FILE_ABS"
        then
            FAILURE="Pagecounts files do not match"
        fi
    fi

    # Checking projectcounts file
    if [ -z "$FAILURE" ]
    then
        if ! diff -Naur dumps/projectcounts-* "$EXPECTED_PROJECTCOUNTS_FILE_ABS"
        then
            FAILURE="Projectcounts files do not match"
        fi
    fi

    # Checking collector's stdout
    if [ -z "$FAILURE" ]
    then
        if [ -s "$COLLECTOR_STDOUT_FILE_ABS" ]
        then
            FAILURE="collector's stdout was not empty."
        fi
    fi

    # Checking collector's stderr
    if [ -z "$FAILURE" ]
    then
        if [ "$(grep -v '^\.temp\.db: unable to flush page: 1$' $COLLECTOR_STDERR_FILE_ABS | wc -l)" != 0 ]
        then
            FAILURE="collector's stderr contained uncanny lines."
        fi
    fi

    # Cleanup
    popd >/dev/null
    rm -rf "$TEST_TMP_DIR_ABS"

    # Flagging test status
    if [ -z "$FAILURE" ]
    then
        mark_test_passed
    else
        mark_test_failed "$FAILURE"
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

# Around Bug 58316 people asked in private whether maybe
# webstatscollector mangles encoding. So let's make sure
# webstatscollector does not mangle encoding.
assert_counted     'http://en.wikipedia.org/wiki/Robinson_Can%C3%B3' 'en' 'Robinson_Can%C3%B3'
assert_counted     'http://en.wikipedia.org/wiki/Robinson_Can\xC3\xB3' 'en' 'Robinson_Can\xC3\xB3'
assert_counted     'http://en.wikipedia.org/wiki/Robinson_Canó' 'en' 'Robinson_Canó'

#(Bug 66352) Requests for “undefined" and "Undefined" are fo the by
#far biggest part artifacts of JavaScript issues, so we no longer
#count them.
assert_not_counted     'http://en.wikipedia.org/wiki/undefined'
assert_not_counted     'http://en.wikipedia.org/wiki/Undefined'
assert_counted     'http://en.wikipedia.org/wiki/UndefinedFoo' 'en' 'UndefinedFoo'
assert_counted     'http://en.wikipedia.org/wiki/FooUndefined' 'en' 'FooUndefined'
assert_not_counted     'http://en.m.wikipedia.org/wiki/undefined'
assert_not_counted     'http://en.m.wikipedia.org/wiki/Undefined'
assert_counted     'http://en.m.wikipedia.org/wiki/UndefinedFoo' 'en.mw' 'en'
assert_counted     'http://en.m.wikipedia.org/wiki/FooUndefined' 'en.mw' 'en'

# Project extraction
assert_counted     'http://commons.wikimedia.org/wiki/Foo' 'commons.m' 'Foo'
assert_not_counted 'http://foo.wikimedia.org/wiki/Foo'
assert_counted     'http://www.wikidata.org/wiki/Foo' 'www.wd' 'Foo'
assert_not_counted 'http://foo.bar.org/wiki/Baz'

# Status codes
LOG_LINE_STATUS="hit/200"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/200'  'en' 'hit/200'
LOG_LINE_STATUS="miss/200" ; assert_counted 'http://en.wikipedia.org/wiki/miss/200' 'en' 'miss/200'
LOG_LINE_STATUS="pass/200" ; assert_counted 'http://en.wikipedia.org/wiki/pass/200' 'en' 'pass/200'
LOG_LINE_STATUS="hit/206"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/206'  'en' 'hit/206'
LOG_LINE_STATUS="hit/300"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/300'  'en' 'hit/300'
LOG_LINE_STATUS="hit/301"  ; assert_not_counted 'http://en.wikipedia.org/wiki/hit/301'
LOG_LINE_STATUS="hit/302"  ; assert_not_counted 'http://en.wikipedia.org/wiki/hit/302'
LOG_LINE_STATUS="hit/303"  ; assert_not_counted 'http://en.wikipedia.org/wiki/hit/303'
LOG_LINE_STATUS="hit/304"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/304'  'en' 'hit/304'
LOG_LINE_STATUS="hit/305"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/305'  'en' 'hit/305'
LOG_LINE_STATUS="hit/400"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/400'  'en' 'hit/400'
LOG_LINE_STATUS="hit/401"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/401'  'en' 'hit/401'
LOG_LINE_STATUS="hit/402"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/402'  'en' 'hit/402'
LOG_LINE_STATUS="hit/403"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/403'  'en' 'hit/403'
LOG_LINE_STATUS="hit/404"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/404'  'en' 'hit/404'
LOG_LINE_STATUS="hit/405"  ; assert_counted 'http://en.wikipedia.org/wiki/hit/405'  'en' 'hit/405'


# Idiosyncrasies ---------------------------------------------------------------
# Here, we document some idiosyncrasies of webstatscollector.
# We might wish to change/fix them, but that would require all
# consumers of those files to adapt their software. And it would make
# comparison between files harder. So let's at least call them out for
# now.

# Idiosyncrasy #1 Pageviews to mobile enwiki, are only counted for
# .mw, not for plain enwiki. And this counting is not "per page", but
# "per language".

assert_counted 'http://en.m.wikipedia.org/wiki/Idiosyncrasy/Page_on_MobileEnwikiSite_only_counted_titleless_for_en.mw' 'en.mw' 'en'

# Idiosyncrasy #2 While "en.mw" might suggest to be thought of as
# "English mobile wikipedia", it is rather "English mobile sites". So
# it includes for example hits to enwikivoyage.
assert_counted 'http://en.m.wikivoyage.org/wiki/Idiosyncrasy/Page_on_MobileEnwikivoyageSite_only_counted_titleless_for_en.mw' 'en.mw' 'en'

# Idiosyncrasy #3 Languages in domain names are considered case
# sensitive.
assert_counted 'http://En.wikipedia.org/wiki/Idiosyncrasy/Case_sensitive_languages' 'En' 'Idiosyncrasy/Case_sensitive_languages'

# Idiosyncrasy #4 Some internal IPv4 IPs are not counted
# altogether. This gets in the way for SSL requests, and makes it
# necessary that the logs from the SSL terminators get fed into the
# filter process too.
# First some internal IP addresses covered by 'filter'.
LOG_LINE_IP="208.80.152.1" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/208.80.152.x'
LOG_LINE_IP="208.80.153.2" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/208.80.153.x'
LOG_LINE_IP="208.80.154.3" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/208.80.154.x'
LOG_LINE_IP="208.80.155.3" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/208.80.155.x'
LOG_LINE_IP="91.198.174.5" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/91.198.141.x'
LOG_LINE_IP="10.128.0.108" ; assert_not_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/10.128.0.x'

# Then some internal IP addresses not covered by 'filter'.
LOG_LINE_IP="198.35.26.6" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/198.35.26.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/198.35.26.x_not_covered'
LOG_LINE_IP="198.35.27.7" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/198.35.27.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/198.35.27.x_not_covered'
LOG_LINE_IP="185.15.56.7" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/185.15.56.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/185.15.56.x_not_covered'
LOG_LINE_IP="185.15.57.8" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/185.15.57.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/185.15.57.x_not_covered'
LOG_LINE_IP="185.15.58.9" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/185.15.58.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/185.15.58.x_not_covered'
LOG_LINE_IP="185.15.59.10" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/185.15.59.x_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/185.15.59.x_not_covered'
LOG_LINE_IP="2620:0:860::11" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/2620:0:860::_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/2620:0:860::_not_covered'
LOG_LINE_IP="2a02:ec80::12" ; assert_counted 'http://en.wikipedia.org/wiki/Idiosyncrasy/Do_not_count_internal/2a02:ec80::_not_covered' 'en' 'Idiosyncrasy/Do_not_count_internal/2a02:ec80::_not_covered'

# Idiosyncrasy #5 Pages are only counted through '/wiki' URLs.
# 'index.php' is not counted.
assert_not_counted 'http://en.wikipedia.org/w/index.php?title=Main_Page'

# Idiosyncrasy #6 Zero does not get counted at all
assert_not_counted 'http://en.zero.wikipedia.org/wiki/Main_Page'

# Idiosyncrasy #7 Desktop site for wikimediafoundation.org does not
# get counted, as it is language-less. Mobile site does get counted
# however, with "m" as language.
assert_not_counted 'http://wikimediafoundation.org/wiki/Home'
assert_counted 'http://m.wikimediafoundation.org/wiki/Home' 'm.f' 'Home'


# -- Finally, some collector checks --------------------------------------------

for COLLECTOR_TEST_SPEC_DIR_RELS in collector-tests/*
do
    test_collector "$COLLECTOR_TEST_SPEC_DIR_RELS"
done

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
