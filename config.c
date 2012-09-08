#include<stdio.h>
#include<stdlib.h>
#include"config.h"
#include<string.h>

// the configuration file is always at the location that the program starts

int oconfig_init(oconfig_t** config){


*config=(oconfig_t *) malloc(sizeof(oconfig_t));

//Open the configuration file
FILE *fconfig=fopen("./config","r");

int line_position=0;
while((fgets((*config)->line[line_position],1000,fconfig))&&(line_position<30)){

//TODO here I write the structure of the configuration

//first line is the host

//host comma separated host:port pairs, each corresponding to a zk
//server. e.g. "127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002"

line_position++;
(*config)->n_lines=line_position;
}
fclose(fconfig);

}

int oconfig_host(oconfig_t *config,char *host){
memcpy(host,config->line[0],1000);
}

int oconfig_recv_timeout(oconfig_t *config,int *timeout){
*timeout=atoi(config->line[1]);
}
//computer name
int oconfig_comp_name(oconfig_t *config,char *comp_name){
memcpy(comp_name,config->line[2],1000);
}
//resource unique name
int oconfig_res_name(oconfig_t *config,char *name){
memcpy(name,config->line[3],1000);
}
//n_pieces
int oconfig_n_pieces(oconfig_t *config,int *n_pieces){
*n_pieces=atoi(config->line[4]);
}
//port
int oconfig_port(oconfig_t *config,char *port){
memcpy(port,config->line[5],1000);
}




int oconfig_destroy(oconfig_t *config){

free(config);

}


