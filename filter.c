#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <grp.h>
#include <stdbool.h>

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

char *wmwhitelist[] = {"commons","meta","incubator","species","strategy","outreach","usability","quality"};
bool check_wikimedia(char *language) {
	char **p=wmwhitelist;
	for (;*p;p++) {
		if (!strcmp(*p,language))
			return true;
	}
	return false;
}

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

char *dupes[] = {
		 "10.128.0.",
		 "208.80.152.",
		 "208.80.153.",
		 "208.80.154.",
		 "208.80.155.",
		 "91.198.174.",
		 NULL};

bool check_ip(char *ip) {
	char **prefix=dupes;
	for (;*prefix;prefix++) {
		if (!strncmp(*prefix,ip,strlen(*prefix)))
			return false;
	}
	return true;
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
		{"wikivoyage", ".voy", NULL},
		{"wikimediafoundation", ".f", NULL},
		{"wikidata", ".wd", NULL},
		{NULL, NULL, NULL}
	}, *project;

struct info {
	char *ip;
	char *size;
	char *language;
	char *project;
	char *title;
	char *suffix;
} info;

void replace_space(char *url) {
	int len = strlen(url);
	if (len==0) {
		return;
	}

	int i;
	for (i = 0; i < len; i++){
		if (url[i] == ' '){
			url[i] = '_';
		}
	}
}

bool parse_url(char *url, struct info *in) {
	if (!url)
		return false;
	char *host, *dir;

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

	if (!in->language || !in->project )
		return false;

	if (!strncmp(in->title,"Special:CentralAutoLogin/", 25))
		return false; /* Special:CentralAutoLogin/.* are no real page views.*/

	if (!strcmp(in->title,"undefined"))
		return false; /* undefined is typically an JavaScript artifact. */

	if (!strcmp(in->title,"Undefined"))
		return false; /* Undefined is typically a redirect "undefined" */

	if (!strcmp(in->project,"m")) {
		in->project = "m.wikipedia";
		in->title = in->language;
		return true;
	} else {
	if (strcmp(TAIL,"org"))
		return false;
	if (in->language && in->project)
		return true;
	else
		return false;
  }
}

bool check_project(struct info *in) {
	const struct project *pr=projects;
	for (;pr->full;pr++) {
		if (!strcmp(in->project,pr->full)) {
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

int main(int ac, char **av) {
	char line[LINESIZE];
	__gid_t gidlist[] = {65534};

	chroot("/tmp/");
	chdir("/");
	setgid(65534);
	setgroups(1,gidlist);
	setuid(65534);

	char *url;
	while (fgets(line,LINESIZE-1,stdin)) {
		bzero(&info,sizeof(info));
		/* Tokenize the log line */
		TOKENIZE(line,"\t"); /* server */
				FIELD; /* id? */
				FIELD; /* timestamp */
				FIELD; /* ??? */
		info.ip=	FIELD; /* IP address! */
				FIELD; /* status */
		info.size=	FIELD; /* object size */
				FIELD;
		url=		FIELD;
		if (!url || !info.ip || !info.size)
			continue;
		replace_space(url);
		if (!check_ip(info.ip))
			continue;
		if (!parse_url(url,&info))
			continue;
		if (!check_project(&info))
			continue;
		printf("%s%s 1 %s %s\n",info.language, info.suffix, info.size, info.title);
	}
	return 0;
}
