#ifndef _OCTOPUS_ZOOKEEPER_H_
#define _OCTOPUS_ZOOKEEPER_H_

#include<zookeeper/zookeeper.h>
#include"config.h"
#include"worker.h"

#define _LL_CAST_ (long long)

typedef struct {
unsigned int id; //used to id updates
char *key;  //computer + res_name that is also the id of all communication to the worker 
//contains null

//these are used to find the changes that happened to the children
struct String_vector computers;
struct String_vector *resources;
}oz_updater_t;

typedef struct{
zhandle_t *zh;
oconfig_t *config;
void *pub;
void *router;
oz_updater_t updater;
workers_t *workers;
}ozookeeper_t;

typedef struct{
ozookeeper_t *ozookeeper;
int retries;
int max_retries;
oconfig_t *config;
}global_watcherctx_t;


//initialize the ozookeeper object
int ozookeeper_init(ozookeeper_t **ozookeeper, oconfig_t *config,global_watcherctx_t *watcherctx,void *pub, void * router);


int ozookeeper_set_zhandle(ozookeeper_t *ozookeeper, zhandle_t *zh);

int ozookeeper_zhandle(ozookeeper_t *ozookeeper, zhandle_t **zh);

int ozookeeper_destroy(ozookeeper_t *ozookeeper);

void global_watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context);
//initialize the watcherctx object
int global_watcherctx_init(global_watcherctx_t **watcherctx,oconfig_t *config);

int global_watcherctx_destroy(global_watcherctx_t *watcherctx);

// doesnt allocate memory
int oz_updater_init(oz_updater_t *updater);

int oz_updater_destroy(oz_updater_t *updater);

int oz_updater_key (oz_updater_t * updater, char *key);

int oz_updater_free_key (oz_updater_t * updater);

int oz_updater_destroy (oz_updater_t * updater);

#endif

