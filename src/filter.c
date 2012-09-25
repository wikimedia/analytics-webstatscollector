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

char *wmwhitelist[] = {"commons","meta","incubator","species","strategy","outreach","planet","blog",NULL};



struct info {
		char *ip;
		char *size;
		char *language;
		char *project;
		char *title;
		char *suffix;
	} info;

typedef struct {
  bool has_dir;
  bool has_title;
  char dir[2000];
  char title[7000];

  char host_parts[30][200];
  int  n_host_parts;

} url_s ;

bool check_wikimedia(char *language) {
	char **p=wmwhitelist;
	for(;*p;p++) {
		if(!strcmp(*p,language))
			return true;
	}
	return false;
}


/**
  *  The following one-liner produces the lookup table
  *  perl -e '@lookup=map { 0+$_; } split(//,0 x 255); $lookup[ord("$_")] = $_ for 0..9; $lookup[ord("$_")] = ord("$_")-ord("A")+10 for "A".."F"; $lookup[ord("$_")] = ord("$_")-ord("a")+10 for "a".."f";    print join(",",@lookup);  '
  *  Basically this is a look-up table tha translates '0'->'9'  into  0->9
  *  and 'A'->'B' to 10->15 (also for 'a'->'b' lowercase)
  *  This is because for example if we have %B5 in an URL, that means we want to get rid of the '%' and convert B5 from hex(base 16) to dec(base 10) like this:
  *  B is 11 and 5 is 5, so we'll have B5 => 16*11 + 5 = 176 + 5 = 181
  *  Here it's important to note that inside the url_s struct we have a member called title and that's a char array with 6000 elements.
  *  We have that many elements because for example if you find in the logs http://zh.wikipedia.org/wiki/%E5%BF%83%E4%B9%8B%E8%B0%B7  this will be parsed to 心之谷
  *  So that is the actual title for that particular article and that's what decode_title(char *) does, it  decodes the URL encoding into readable characters.
  */
int decode_hex2dec[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#define decode_title_lookup(first_char,second_char)  16*first_char

/* decode html url encoding in title inplace */

void decode_title(char *title) {
  char result[3000];

  char *r = result;
  char *t = title ;
  for(;*t;t++) {
    if(*t == '%') {
      char d1 = decode_hex2dec[*(t+1)];
      char d2 = decode_hex2dec[*(t+2)];
      *r = d2 + (d1<<4);
      t+=2;
    } else {
      *r = *t;
    };
    r++;
  };
  *r='\0';
  strcpy(title,result);
};





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



/*
 * URL is broken down into relevant components
 *
 */

void explode_url(char *url,url_s *u) {
  u->has_dir       = false;
  u->has_title     = false;
  u->n_host_parts  = 0;

  if(       strncmp(url,"http://" ,7) == 0) {
    url+=7;
  } else if(strncmp(url,"https://",8) == 0) {
    url+=8;
  } else {
    return;
  };


  char *hostname_iter = url;
  size_t len_part;
  size_t len_until_params = strcspn(hostname_iter,"?#");
  *(url+len_until_params) = '\0'; // ignore any params in the url


  bool end_url      = false;
  bool found_dir    = false;

  /* this part will split the domain name into fragments into u.hostname_parts */
  while(!found_dir && !end_url && (len_part = strcspn(hostname_iter,"./"))) {
    /* do different things depending on separator that was found */
    switch(*(hostname_iter+len_part)) {
      case '/':
        // copy last domain name part where it belongs
        strncpy(u->host_parts[u->n_host_parts++],hostname_iter,len_part);
        u->host_parts[u->n_host_parts][len_part+1] = '\0';//strncpy does not do string endings
        hostname_iter += len_part;
        found_dir = true;
      case 0  :
        end_url   = true;
        break;
      default:
        strncpy(u->host_parts[u->n_host_parts++],hostname_iter,len_part);
        u->host_parts[u->n_host_parts][len_part+1] = '\0';//strncpy does not do string endings
        hostname_iter += len_part+1; // jump over separator
    };
  };

  /* if a '/' is found that means a directory follows, also check if there are any trailing characters after it */
  if(found_dir && *(hostname_iter+1)) {
    u->has_dir = true;
    strcpy(u->dir,hostname_iter);
    if(strncmp(u->dir,"/wiki/",6)==0) {
      u->has_title = true;

      strcpy(u->title,u->dir+6);
      decode_title(u->title); //optional, can be removed, this just turns titles like %B5%EF  into readable characters
    };
  };


  // for debug
/*
 *  int i;
 *  for(i=0;i < u->n_host_parts;i++) {
 *    printf("host_part=%s\n",u->host_parts[i]);
 *  };
 *
 *  printf("dir=%s\n",u->dir);
 *  printf("title=%s\n",u->title);
 */

  return;
}


bool parse_url(char *url, struct info *in,url_s *u) {
	char  *host;
	char  *dir;

	if (!url)
		return false;

        explode_url(url,u);

        /* this branch is for urls with /wiki/ inside them */
        if(u->has_title) {
          in->title = u->title;

          in->project  = u->host_parts[u->n_host_parts-2];
          in->language = u->host_parts[0];

          if(strcmp(u->host_parts[0],"m") ==0) {
            in->project = "m.wikipedia";
            return true;
          } else if(strcmp(in->language,"wikimediafoundation")==0) {
            in->project  = in->language;
            in->language =  "blog";
            return true;
          } else {

            if(strlen(in->project) > 0 && strlen(in->language) > 0) {
              return true;
            } else {
              return false;
            }
          };

        } else {
          /* this branch is for special cases */
          in->project  = u->host_parts[u->n_host_parts-2];
          in->language = u->host_parts[0];


          /*
           *int i;
           *for(i=0;i < u->n_host_parts;i++) {
           *  printf("host_part=[%s]\n",u->host_parts[i]);
           *};
           */


          if(
            // for *.planet.wikimedia.org
            strcmp(u->host_parts[1],"planet"   )==0 &&
            strcmp(u->host_parts[2],"wikimedia")==0
          ) {
            in->language = u->host_parts[1];
            strcpy(u->title,"main");
            in->title = u->title;
            return true;
            // for blog.wikimedia.org
          } else if(strcmp(u->host_parts[1],"wikimedia")==0 && strcmp(u->host_parts[0],"blog")==0 ) {
            strcpy(u->title,"main");
            //getting title of blog post
            int len_dir = strlen(u->dir);
            if(u->dir[len_dir-1] == '/') {
              u->dir[len_dir-1] = '\0';
              len_dir--;
            };
            char *title=u->dir + len_dir;
            while(*--title != '/');
            in->title = title+1;
            return true;
            //for wikimediafoundation.org
          } else if(strcmp(in->project,"wikimediafoundation")==0) {
            strcpy(u->title,"main");
            in->title = u->title;
            return true;
          };


          return false;
        };

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


	setgid(65534);
	setgroups(1,gidlist);
	setuid(65534);

	while(fgets(line,LINESIZE-1,stdin)) {
                url_s u;
		bzero(&info,sizeof(info));
		bzero(&u   ,sizeof(u));
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

		if (!parse_url(url,&info,&u)) {
			continue;
                }
		if (!check_ip(info.ip)) {
			continue;
                }
		if (!check_project(&info)) {
			continue;
                }
		if (user_agent_is_bot(crawlers, ua)) {
			if (test){
				printf("1000 %s%s_bot 1 %s %s\n",info.language, info.suffix, info.size, info.title);
			} else {
				printf("1 %s%s_bot 1 %s %s\n",info.language, info.suffix, info.size, info.title);
			}
		}
		if (test) {
			printf("1001 %s%s 1 %s %s\n",info.language, info.suffix, info.size, info.title);
		} else {
			printf("1 %s%s 1 %s %s\n",info.language, info.suffix, info.size, info.title);
		}

	}
	return 0;
}

