/*
 Poor man's XML exporter. :)

 Author: Domas Mituzas ( http://dammit.lt/ )

 License: public domain (as if there's something to protect ;-)

 $Id: export.c 12389 2006-01-04 11:21:01Z midom $

*/
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <db.h>
#include "collector.h"

void dumpData(FILE *fd, DB *db) {
	DBT key,data;
	DBC *c;

        #if USE_UTF8 == 1
          char *toCode="UTF8"; // <======= this is what I want to convert it to
          /*char *fromCode="ISO-LATIN-1";//ISO-8859-1";*/
          char *fromCode="ASCII";// <===== I presume after removing % and converting from hex to dec I will get ASCII encoding
        #endif

	char *p,*project,*page;

	struct wcstats *entry;

	bzero(&key,sizeof(key));
	bzero(&data,sizeof(data));
	
	db->cursor(db,NULL,&c,0);


        #if USE_UTF8 == 1
          iconv_t it=iconv_open(toCode, fromCode);
        #endif


        // ok so what happens is
	while(c->c_get(c, &key, &data, DB_NEXT )==0) {
		entry=data.data;
		p=key.data;
		/* project points to project! */	
		project=strsep(&p,":");
		
		/* Can just use p afterwards, but properly named variable sometimes helps */
		page=p;
		
		/* Get EVENT */
                #if USE_UTF8 == 1
                   //over here is the conversion to utf8, the
                   // line that's supposed to go in dumps/pagecounts..  is formatted here
                   // and then converted to UTF8 because 
                    char result[10000];
                    char source[10000];
                    if(it!=(iconv_t)-1) {

                      strcpy(result,"    ");
                      sprintf(source,"%s %s %llu %llu\n",
                              project,(page?page:"-"),
                              entry->wc_count, entry->wc_bytes);
                      size_t inbytesleft = strlen(source);
                      size_t outbytesleft = inbytesleft*6;
                      if(iconv(it,&source,&inbytesleft,&result,&outbytesleft)!=-1) {
                        // all ok, ready to write to file
                        fprintf(fd,"%s",result);
                      } else {
                        perror("iconv()");
                      };

                    } else {
                      perror("iconv_open()");
                    };
                    
                #else
                    fprintf(fd,"%s %s %llu %llu\n",
                            project,(page?page:"-"),
                            entry->wc_count, entry->wc_bytes);

                #endif
	};


        #if USE_UTF8 == 1
          iconv_close(it);
        #endif
	c->c_close(c);
}

