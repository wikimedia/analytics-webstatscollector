#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <grp.h>

/* 

Modified on Jun 4, 2010 by hcatlin to 
now process en.m.wikipedia.org requests into
the mobile project, with the title being the
language code.

For instance, it might output "en.mw 1 1213 en"

This way, we aren't tracking every page title in
mobile.

*/

#define LINESIZE 4096
char *_sep, *_lasttok, *_firsttok;
#define TOKENIZE(x,y) _lasttok=NULL; _sep=y; _firsttok=strtok_r(x,y,&_lasttok);
#define FIELD strtok_r(NULL,_sep,&_lasttok)
#define TAIL _lasttok
#define HEAD _firsttok

/* IP addresses from which duplicate requests originate 
* Correspondence with Mark Bergsma on IRC.
* diederik: what is exactly the issue?
* mark: squids requesting from eachother
* diederik: 1) is this still an issue (mentioned on line 60)?
* mark: only 208.80. and 91.198.174 are still relevant
* removed IP blocks:
* "145.97.39.",
* "66.230.200.",
* "211.115.107.",
*/

char *dupes[] = {"208.80.152.",
				"208.80.153.",
				"208.80.154.",
				"208.80.155.",
				"91.198.174.",
				NULL};

char *wmwhitelist[] = {"commons","meta","incubator","species","strategy","outreach","usability","quality"};


struct info {
		char *ip;
		char *size;
		char *language;
		char *project;
		char *title;
		char *suffix;
	} info;

bool check_wikimedia(char *language) {
	char **p=wmwhitelist;
	for(;*p;p++) {
		if(!strcmp(*p,language))
			return true;
	}
	return false;
}

const struct project {
	char *full;
	char *suffix;
	bool (*filter)(char *);
	} projects[] = {
		{"wikipedia","",NULL},
		{"wiktionary",".d",NULL},
		{"wikinews",".n",NULL},
		{"wikimedia",".m",check_wikimedia},
		{"wikibooks",".b",NULL},
		{"wikisource",".s",NULL},
		{"mediawiki",".w",NULL},
		{"wikiversity",".v",NULL},
		{"wikiquote",".q",NULL},
		{"m.wikipedia", ".mw", NULL},
		{"wikimediafoundation", ".f", NULL},
		{NULL},
	}, *project;


void signal_callback_handler(int signum) {
	fprintf(stderr, "Caught signal %d\n",signum);
	/* Cleanup and close up stuff here */

	/* Terminate program */
	exit(signum);
}


bool check_ip(char *ip) {
	char **prefix=dupes;
	for (;*prefix;prefix++) {
		if(!strncmp(*prefix,ip,strlen(*prefix)))
			return false;
	}
	return true;
}


bool parse_url(char *url, struct info *in) {
	char *host;
	char *dir;

	if (!url)
		return false;

	TOKENIZE(url,"/"); /* http: */
	host=FIELD;

	dir=FIELD;
	if (!dir)
		return false;
	if (strcmp(dir,"wiki"))
		return false; /* no /wiki/ part :( */
	in->title=TAIL;
	TOKENIZE(in->title,"?#");

	TOKENIZE(host,".");
	in->language=HEAD;
	in->project=FIELD;
	if(strcmp(in->project,"m")==0) {
    	in->project = "m.wikipedia";
    	/*in->title = in->language; */
    	return true;
	} else if(strcmp(in->language, "wikimediafoundation")==0){
		in->project = in ->language;
		in->language = "blog";
		return true;
	} else {
		if (in->language && in->project)
			return true;
		else
			return false;
	}
}

bool user_agent_is_bot(char *crawlers[], const char *user_agent) {
	/*
	 * If user agent contains either 'bot', 'spider', 'crawler' or 'http'
	 * then we classify this request as a bot request, else it is a regular
	 * request.
	 */
	if(user_agent){
		int i=0;
		char *result;
		for (;;){
			if (crawlers[i]) {
				result = strstr(user_agent, (char *)crawlers[i]);
				if (result){
					return true;
				}
			} else {
				break;
			}
			i++;
		}
	}
	return false;
}

bool check_project(struct info *in) {
	const struct project *pr=projects;
	for(;pr->full;pr++) {
		if(!strcmp(in->project,pr->full)) {
			in->suffix=pr->suffix;
			/* Project found, check if filter needed */
			if (pr->filter)
				return pr->filter(in->language);
			else
				return true;
		}
	}
	return false;
}

int main(int argc, char **argv) {
	int c;
	int test = 0;
	int retval;
	static struct option long_options[] = {
			{"test", no_argument, 0, 't'},
			{0,0,0,0}
	};

	char *url;
	char *ua;

	char line[LINESIZE];
	const gid_t gidlist[] = {65534};

	char *crawlers[5];

	signal(SIGINT, signal_callback_handler);

	while((c = getopt_long(argc, argv, "t", long_options, NULL)) != -1) {
		switch(c)
			{
		case 't':
			test = 1;
			break;

		default:
			break;
			}
	}


	/* initialize the crawlers */
	crawlers[0] = "bot";
	crawlers[1] = "spider";
	crawlers[2] = "crawler";
	crawlers[3] = "http";
	crawlers[4] = NULL;
	/* end of initialization */


	retval = chroot("/tmp/");
	if(retval==-1){
		exit(-1);
	}

	retval =chdir("/");
	if (retval == -1) {
		exit(-1);
	}
	setgid(65534);
	setgroups(1,gidlist);
	setuid(65534);

	while(fgets(line,LINESIZE-1,stdin)) {
		bzero(&info,sizeof(info));
		/* Tokenize the log line */
		TOKENIZE(line," "); /* server */
					FIELD; /* Sequence number */
					FIELD; /* timestamp */
					FIELD; /* Request service time in ms */
		info.ip=	FIELD; /* IP address! */
					FIELD; /* status */
		info.size=  FIELD; /* object size */
					FIELD; /* Request method */
		url=		FIELD; /* full url */
					FIELD; /* Squid hierarchy status, peer IP */
					FIELD; /* MIME content type */
					FIELD; /* Referer header */
					FIELD; /* X-Forwarded-For header */
		ua=			FIELD; /* user agent string */

		if (!parse_url(url,&info))
			continue;
		if (!check_ip(info.ip))
			continue;
		if (!check_project(&info))
			continue;
		if (user_agent_is_bot(crawlers, ua)) {
			if (test){
				printf("1000 %s%s_bot 1 %s %s\n",info.language, info.suffix, info.size, info.title);
			} else {
				printf("%s%s_bot 1 %s %s\n",info.language, info.suffix, info.size, info.title);
			}
		}
		if (test) {
			printf("1001 %s%s 1 %s %s\n",info.language, info.suffix, info.size, info.title);
		} else {
			printf("%s%s 1 %s %s\n",info.language, info.suffix, info.size, info.title);
		}
	}
	return 0;
}

