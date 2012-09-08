#ifndef OCTOPUS_CONFIG_H_
#define OCTOPUS_CONFIG_H_


//first line is the host
//comma separated host:port pairs, each corresponding to a zk
//server. e.g. "127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002"


typedef struct {

char line[30][1000];  //all lines have 1000 max char including null
int n_lines;  //maximum lines are 30

} oconfig_t;

int oconfig_init(oconfig_t** config);
//1st line
int oconfig_host(oconfig_t *config,char *host);
int oconfig_destroy(oconfig_t* config);
//2nd line
int oconfig_recv_timeout(oconfig_t *config,int *timeout);

//3rd line 
//unique computer name
int oconfig_comp_name(oconfig_t *config,char *comp_name);

//4th line
//unique resource name
int oconfig_port(oconfig_t *config,char *name);

//5th line
//number of pieces in the hash ring
int oconfig_n_pieces(oconfig_t *config,int *n_pieces);

//port
int oconfig_port(oconfig_t *config,char *port);

#endif
