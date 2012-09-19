/* Collector structures and some headers and some data */
/* $Id: collector.h 12318 2005-12-31 15:34:46Z midom $ */

#include <stdio.h>
#include <db.h>


/**
  * Note: 
  *
  *   Switch to DEBUG = 0 when compiling this for use in production.
  *   DEBUG = 1 is just used for development and testing
  *
  */
#define DEBUG 1

#if DEBUG == 1
  #define PERIOD 60
#else
  #define PERIOD 3600
#endif


#define PREFIX "dumps/"

DB *db;
DB *aggr;

/* Teh easy stats */
struct wcstats {
	unsigned long long wc_count;
	unsigned long long wc_bytes;
};

void dumpData(FILE *, DB *);

struct dumperjob {
	char *prefix;
	DB *db;
	time_t time;
};
