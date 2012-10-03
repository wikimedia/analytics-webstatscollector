/*
 This is a daemon, that sits on (haha, hardcoded) port 3815,
 receives profiling events from squid log aggregators. 
 and places them into BerkeleyDB file. \o/

 Then it should rotate everything once an hour, 
 producing per-hourly aggregates for further filtering. 

 Author: Domas Mituzas ( http://dammit.lt/ )

 License: public domain (as if there's something to protect ;-)

 $Id: collector.c 12389 2006-01-04 11:21:01Z midom $

 The general workflow of webstatscollector is as follows:
 squids -> udp2log -> filter -> log2udp -> collector -> files



 Control flow:

   This is an UDP server which accepts connections.
   It receives data as space separated lines which contain log data.
   Basically, it counts request sizes per project&title and per project.
   These counts are stored in 2 Berkeley DB databases on disk.
   One is called db and the other aggr (and they are stored on disk in the files and the filenames for these files are
                                       generated based on the time they were created, see generate_new_db_name )

   Each argv_period seconds it will fire up a SIGALRM which causes the poll in the main loop to return with EINTR
   and causes control flow to enter produceDump(..) which will swap the existing 2 dbs with two new empty dbs and it will fire up two 
   threads(one for each old db) which will call will call dumpData(..) and inside dumpData, there is a loop which iterates through 
   all the rows in the DB and writes pagecounts and projectcounts files on disk.

 Developing:

   In order to develop, you can start up you can compile everything with  ./build.sh
   and then run   cat example2.log | ./filter | nc -u 127.0.0.1 3815
   This requires you to have the collector running because what the above command will do is send the data that filter outputs
   to udp port 3815 on your local machine.
   Make sure you have a dumps directory.

*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <db.h>
#include <getopt.h>
#include <pthread.h>
#include "collector.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>


#define FILENAME_DB_DB    ".temp.db.%d.db"
#define FILENAME_DB_AGGR  ".temp.aggr.%d.db"

int argv_port   = -1;
int argv_period = -1;
char old_filename_db[  100];
char old_filename_aggr[100];

char *VERSION="development-placeholder-version";
void print_error(int error){
	char *msg = strerror(error);
	fprintf(stderr,"Encountered error:%s\n", msg);
}

inline char *generate_new_db_name(char *where_to_write,char *dbname) {
  if(!(dbname && where_to_write)) {
    fprintf(stderr,"Error: generate_new_db_name");
    exit(-2);
  };
  sprintf(where_to_write,".temp.%s.%d.db",dbname,(unsigned int)time(NULL));

  return where_to_write;
}

void hup();
void alarmed();
void die();
void child();
void truncatedb();


DB * initEmptyDB();
void handleMessage(char *,ssize_t );
void handleConnection(int);
void produceDump(char *);
void increaseStatistics(DB *, char *, struct wcstats * );

int needdump=0;


#define DIRECTORY_PERMISSIONS_ERROR   2
#define DIRECTORY_DOESNT_EXIT         1
#define DIRECTORY_EXISTS_AND_WRITABLE 0

int directory_exists(char *dname) {
  struct stat st;
  int ret;

  if (stat(dname,&st) != 0) {
    return DIRECTORY_DOESNT_EXIT;
  }

  ret = S_ISDIR(st.st_mode);
  if(!ret) {
    return DIRECTORY_DOESNT_EXIT;
  };

  if(st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)) {
    if(access(dname, W_OK)) {
      return DIRECTORY_PERMISSIONS_ERROR;
    } else {
      // already exists but writable
      return DIRECTORY_EXISTS_AND_WRITABLE;
    };
  } else {
    return DIRECTORY_EXISTS_AND_WRITABLE;
  };
}


int main(int argc, char **argv) {
	ssize_t l;
	char buf[2000];
	int r;
        int c;
	int retval;
        char path_dumps[600];
        int len_path_dumps = 0;

        #if DEBUG == 1
          fprintf(stderr,"Warning! =>  DEBUG=1\n");
        #endif

	static struct option long_options[] = {
			{"help"    , no_argument       , 0 , 'h'},
			{"version" , no_argument       , 0 , 'v'},
			{"port"    , required_argument , 0 , 'p'},
			{"period"  , required_argument , 0 , 't'},
			{"output"  , required_argument , 0 , 'o'},
			{0,0,0,0}
	};

	while((c = getopt_long(argc, argv, "o:hvp:t:", long_options, NULL)) != -1) {
          switch(c)
          {
            case 'v':
              printf("\nWebStatsCollector - Collector\nVersion=%s\n\n",VERSION);
              exit(0);
            case 'h':
              printf("Usage: collector [OPTION]\n");
              printf("\n");
              printf("  -v, --version        Print the version of collector\n");
              printf("  -h, --help           display this help and exit\n");
              printf("  -p, --port=NUMBER    Port on which collector will listen\n");
              printf("  -t, --period=NUMBER  Period between data dumps, measured in seconds\n");
              printf("  -o, --output=STRING  Path where dumps will be placed. This path must exist.\n");
              printf("\n");
              printf("Collector listens for data on port 3815 on UDP protocol(by default, but can be modified through -p) and\n");
              printf("collects data that comes on that port.\n");
              printf("The data collected must be in the format filter will produce. \n");
              printf("\n");
              exit(0);
            case 'p':
              argv_port = atoi(optarg);
              if(argv_port < 0) {
                printf("Error , invalid port number\n");
                exit(-1);
              };
              printf("Listening on port %d\n",argv_port);
              break;
            case 't':
              argv_period = atoi(optarg);
              if(argv_period < 0) {
                printf("Error , invalid number of seconds\n");
                exit(-1);
              };
              fprintf(stderr,"Period is %d seconds\n",argv_period);
              break;
            case 'o':
              strcpy(path_dumps,optarg);
              len_path_dumps = strlen(path_dumps);

              /* a trailing slash needed, so we make sure we put one there */
              if(len_path_dumps > 0 && path_dumps[len_path_dumps-1] != '/') {
                path_dumps[len_path_dumps] = '/';
                path_dumps[len_path_dumps+1] = '\0';
              };
              break;
            default:
              break;
          }
	}


        /* no -o param was present so we just put the default */
        if(len_path_dumps == 0) {
          strcpy(path_dumps,PREFIX);
        };

        char error_msg[1000];
        /* check if dump directory exists */
        switch(directory_exists(path_dumps)) {
          case DIRECTORY_DOESNT_EXIT:
            sprintf(error_msg,"Error: directory %s not present, create one\n",path_dumps);
            fprintf(stderr,"%s",error_msg);
            exit(-1);
            break;
          case DIRECTORY_PERMISSIONS_ERROR:
            fprintf(stderr,"Error: Directory exists you don't have enough permissions to write to it\n");
            exit(-1);
            break;
          case DIRECTORY_EXISTS_AND_WRITABLE:
            fprintf(stderr,"Will write dumps to %s\n",path_dumps);
            break;
          default:
            fprintf(stderr,"Error: on line %d\n",__LINE__);
            break;
        };

	/* Socket variables */
	int s, exp;
	u_int yes=1;
	int port;
	struct sockaddr_in me, them;
	socklen_t sl;

	struct pollfd fds[2];

	/*Initialization*/{
		#ifdef __linux___
		printf("initialization\n");
		retval = daemon(0, 1);
		if(retval  == -1) {
			fprintf(stderr,"Failed to daemonize.\n");
			exit(EXIT_FAILURE);
		}
		#endif
		setuid(65534);
		setgid(65534);

                argv_port   = (argv_port   == -1) ? 3815   : argv_port  ;
                argv_period = (argv_period == -1) ? PERIOD : argv_period;

		bzero(&me,sizeof(me));
		me.sin_family= AF_INET;
		me.sin_port=htons(argv_port);
		inet_aton("127.0.0.1", &me.sin_addr);
		s=socket(AF_INET,SOCK_DGRAM,0);
		bind(s,(struct sockaddr *)&me,sizeof(me));

		exp=socket(AF_INET,SOCK_STREAM,0);
		setsockopt(exp,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
		bind(exp,(struct sockaddr *)&me,sizeof(me));
		listen(exp,10);

		bzero(&fds,sizeof(fds));

		fcntl(s,F_SETFL,O_NONBLOCK);
		fcntl(exp,F_SETFL,O_NONBLOCK);

		fds[0].fd = s; fds[0].events |= POLLIN;
		fds[1].fd = exp, fds[1].events |= POLLIN;

		db=initEmptyDB(   generate_new_db_name(old_filename_db  ,"db"  )  );
		aggr=initEmptyDB( generate_new_db_name(old_filename_aggr,"aggr")  );

		signal(SIGALRM,alarmed);
		signal(SIGHUP,hup);
		signal(SIGINT,die);
		signal(SIGTERM,die);
		signal(SIGCHLD,child);
		signal(SIGUSR1,truncatedb);

		/* Schedule the dumps */
		alarm(argv_period-(time(NULL)%argv_period));
	}
	/* Loop! loop! loop! */
	printf("Looping\n");
	for(;;) {
		printf("1: here we are again\n");
		r=poll(fds,2,-1);
		if (needdump)
			produceDump(path_dumps);

		/* Process incoming UDP queue */
		while(( fds[0].revents & POLLIN ) &&
			((l=recvfrom(s,&buf,1500,0,NULL,NULL))!=-1)) {
				if (l==EAGAIN)
					break;
				handleMessage((char *)&buf,l);
			}

		/* Process incoming TCP queue - for testing data collection only */
		while((fds[1].revents & POLLIN ) &&
			((r=accept(exp,(struct sockaddr *)&them,&sl))!=-1)) {
				if (r==EWOULDBLOCK)
					break;
				handleConnection(r);
		}
	}
	printf("closing the db's\n");
	db->close(db, 0);
	aggr->close(aggr, 0);

	return(0);
}


/* Decides what to do with incoming UDP message */
void handleMessage(char *buf,ssize_t l) {
	char *p,*pp;
	char project[128];
	char title[1024];
	char keytext[1200];
	int r;



        /** 
          * serial is not used in collector, it's used just for the generate-test-data.pl
          * for debugging purposes
          *
        */

        unsigned long long serial; 

	struct wcstats incoming;

	/* project count bytesize page */
	const char msgformat[]="%llu %127s %llu %llu %1023[^\n]";

	/*
         * field 1: serial number(just for debugging)
	 * field 2: project title with a max of 127 bytes excluding the '\0'
	 * field 3: unsigned long count, by default 1
	 * field 4: unsigned long size of http request
	 * field 5: page title with a max of 1023 bytes excluding the '\0'
	 */
	printf("3: Handling the message\n");

	buf[l]=0;
	pp=buf;

	while((p=strsep(&pp,"\r\n"))) {

		if (p[0]=='\0')
			continue;
		if (!strcmp("-truncate",p)) {
			truncatedb();
			return;
		}
		printf("Got the message: [%s]\n", p);
		bzero(&incoming,sizeof(incoming));
		r=sscanf(p,msgformat,&serial,(char *)&project,
			&incoming.wc_count,
			&incoming.wc_bytes,
			(char *)&title);
		printf("Number of fields: %d\n", r);
		if (r<5)
			continue;
		snprintf(keytext,1199,"%s:%s",project,title);

		increaseStatistics(db,keytext,&incoming);
		increaseStatistics(aggr,project,&incoming);
	}
}

/* Create empty database object - creates file-system backed anonymous BDB handle */
DB * initEmptyDB(char *db_filename) {
	/* Do note - this isn't using global 'db' object */
	int retval;
	DB *db;


	retval = db_create(&db,NULL,0);
        if(retval!=0) {
          print_error(retval);
        };
	db->set_cachesize(db, 0, 1024*1024*1024, 0);
	retval =db->open(db,NULL,db_filename,NULL,DB_BTREE,DB_CREATE|DB_TRUNCATE,0);
        if (retval==-1){
          print_error(retval);
        };

	return db;
}

/* Bump up statistics in specified DB for specified key */
void increaseStatistics(DB *db, char *keytext, struct wcstats *incoming ) {
	struct wcstats *old;
	DBT key,data;

	bzero(&key,sizeof(key));
	key.data=keytext;
	key.size=strlen(keytext)+1;

	bzero(&data,sizeof(data));
	if (db->get(db,NULL,&key,&data,0)==0) {
		/* Update old stuff */
		old=data.data;
		old->wc_count   += incoming->wc_count;
		old->wc_bytes   += incoming->wc_bytes;
		db->put(db,NULL,&key,&data,0);
	} else {
		/* Put in fresh data */
		data.data=incoming;
		data.size=sizeof(*incoming);
		db->put(db,NULL,&key,&data,0);
	}
}

void statsDumper(struct dumperjob * job) {
	DB *db;
	FILE *outfile;
	char filename[1024];
	char tfilename[1024];
	struct tm dumptime;
	db=job->db;
	gmtime_r(&job->time,&dumptime);
	snprintf(filename,1000,"%s-%04d%02d%02d-%02d%02d%02d",
		job->prefix, dumptime.tm_year+1900, dumptime.tm_mon+1, dumptime.tm_mday,
		dumptime.tm_hour, dumptime.tm_min, dumptime.tm_sec);

	snprintf(tfilename,1010,"%s.tmp",filename);
	outfile=fopen(tfilename,"a");
	if (outfile==NULL) {
		fprintf(stderr,"Problem opening file '%s': %d\n",tfilename,errno); die();
	}
	dumpData(outfile,db);
	fclose(outfile);
	rename(tfilename,filename);
	db->close(db,DB_NOSYNC);

        #if DEBUG==1
          fprintf(stderr,"deleting file %s\n",job->db_to_delete);
        #endif


        // delete the database that has just been dumped to disk through dumpData
        unlink(job->db_to_delete);

}

/* The dumps stuff */
void produceDump(char *path_dumps) {
	/* Teh logicz:
		1) Swap with empty databasez - pretty much atomic, can be done in main loop
		2) Spawn background threads to:
			2.1) Dump out the data
			2.2) Destroy old databases
		3) Set the alarm to next argv_period-thetime%argv_period - this will position the alarm at next nice period.
	*/
	static struct dumperjob dumperJob, aggrDumperJob;
	DB *olddb,*oldaggr;

	pthread_t thread_dumper,thread_aggrdumper;
	time_t dumptime=0;

	needdump=0;
	time(&dumptime);

	olddb=db;
	oldaggr=aggr;

        /* after we've unlinked the old dbs we create new ones which take their place for the upcoming produceDump call*/


        strcpy(dumperJob.prefix,path_dumps);
        strcat(dumperJob.prefix,"pagecounts");
	dumperJob.db=olddb;
	dumperJob.time=dumptime;
        strcpy(dumperJob.db_to_delete,old_filename_db);

	pthread_create(&thread_dumper,NULL,(void *)statsDumper, (void *)&dumperJob);

        strcpy(aggrDumperJob.prefix,path_dumps);
        strcat(aggrDumperJob.prefix,"projectcounts");
	aggrDumperJob.db = oldaggr;
	aggrDumperJob.time=dumptime;
        strcpy(aggrDumperJob.db_to_delete,old_filename_aggr);

	pthread_create(&thread_aggrdumper,NULL,(void *)statsDumper,(void *)&aggrDumperJob);
	alarm(argv_period-(dumptime%argv_period));

        #if DEBUG==1
          fprintf(stderr, "Creating DB\n");
        #endif

        generate_new_db_name(old_filename_db  ,"db"   );
	db  =initEmptyDB( old_filename_db   );
        generate_new_db_name(old_filename_aggr,"aggr" );
	aggr=initEmptyDB( old_filename_aggr );

        #if DEBUG==1
          fprintf(stderr,"Finished creating DB\n");
        #endif
}

/* TCP connection handling logic - unsafe dump of data in DB - should be avoided at large datasets */
void handleConnection(int c) {
	FILE *tmp;
	char buf[1024];
	int r;

	shutdown(c,SHUT_RD);

	tmp=tmpfile();
	dumpData(tmp,db);
	rewind(tmp);
	if (fork()) {
		fclose(tmp);
		close(c);
	} else {
		while(!feof(tmp)) {
			r=fread((char *)&buf,1,1024,tmp);
			int retval_wait = write(c,(char *)&buf,r);
		}
		close(c);
		fclose(tmp);
		exit(0);
	}
}

/* Event handling - most of them are not used or should not be used due to reentry possibilities */

void alarmed() {
	alarm(0);
	needdump=1;
}

void hup() {
	/* Do nothing */
}

void die() {
	exit(0);
}

void child() {
	int status;
	wait(&status);
}

void truncatedb() {
	unsigned int count;
	db->truncate(db,NULL,&count,0);
}
