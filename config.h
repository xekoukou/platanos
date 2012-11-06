#ifndef OCTOPUS_CONFIG_H_
#define OCTOPUS_CONFIG_H_


//first line is the host
//comma separated host:port pairs, each corresponding to a zk
//server. e.g. "127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002"


typedef struct
{

    char line[30][1000];	//all lines have 1000 max char including null
    int n_lines;		//maximum lines are 30

} oconfig_t;

void oconfig_init (oconfig_t ** config);

//name of the octopus
void oconfig_octopus (oconfig_t * config, char *octopus);

void oconfig_host (oconfig_t * config, char *host);
void oconfig_destroy (oconfig_t * config);
void oconfig_recv_timeout (oconfig_t * config, int *timeout);

//unique computer name
void oconfig_comp_name (oconfig_t * config, char *comp_name);


#endif
