#include<stdio.h>
#include"config.h"
#include"zookeeper.h"
#include"workers.h"




int main(){

zctx_t *ctx=zctx_new();
void *pub=zsocket_new(ctx,ZMQ_PUB);
void *router=zsocket_new(ctx,ZMQ_ROUTER);


//Open the configuration file
oconfig_t *config;
oconfig_init(&config);

ozookeeper_t * ozookeeper;
global_watcherctx_t *watcherctx;

global_watcherctx_init(&watcherctx,config);
ozookeeper_init(&ozookeeper,config,watcherctx,pub,router);

workers_t *workers;
workers_init(&workers,ozookeeper);


}
