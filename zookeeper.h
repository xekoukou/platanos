#ifndef _OCTOPUS_ZOOKEEPER_H_
#define _OCTOPUS_ZOOKEEPER_H_

#include<zookeeper/zookeeper.h>
#include"config.h"

#define _LL_CAST_ (long long)


typedef struct{
zhandle_t *zh;
oconfig_t *config;
}ozookeeper_t;

typedef struct{
ozookeeper_t *ozookeeper;
int retries;
int max_retries;
oconfig_t *config;
}global_watcherctx_t;

//initialize the ozookeeper object
int ozookeeper_init(ozookeeper_t **ozookeeper, oconfig_t *config,global_watcherctx_t *watcherctx);

int ozookeeper_online(ozookeeper_t *ozookeeper);

int ozookeeper_set_zhandle(ozookeeper_t *ozookeeper, zhandle_t *zh);

int ozookeeper_zhandle(ozookeeper_t *ozookeeper, zhandle_t **zh);

int ozookeeper_destroy(ozookeeper_t *ozookeeper);

void global_watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context);
//initialize the watcherctx object
int global_watcherctx_init(global_watcherctx_t **watcherctx,oconfig_t *config);

int global_watcherctx_destroy(global_watcherctx_t *watcherctx);

#endif


