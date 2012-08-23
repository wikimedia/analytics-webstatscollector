int main(int ac,char **av);
bool check_project(struct info *in);
bool user_agent_is_bot(char crawlers[],char crawlers_length[],int nr_crawlers,char *user_agent);
bool parse_url(char *url,struct info *in);
bool check_ip(char *ip);
extern char *dupes[];
bool check_wikimedia(char *language);
extern char *wmwhitelist[];
extern char *_sep,*_lasttok,*_firsttok;
