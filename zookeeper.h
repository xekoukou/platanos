#ifndef _OCTOPUS_ZOOKEEPER_H_
#define _OCTOPUS_ZOOKEEPER_H_

#include<zookeeper.h>

#define _LL_CAST_ (long long)


typedef struct{
zhandle_t *zh;
}ozookeeper_t;

typedef {
ozookeeper_t *ozookeeper;
int retries;
int max_retries;
config_t *config;
}watcherctx_t;

//initialize the ozookeeper object
int ozookeeper_init(ozookeeper_t **ozookeeper, config_t *config,watcherctx_t *watcherctx);

int ozookeeper_set_zhandle(ozookeeper_t *ozookeeper, zhandle_t *zh);

int ozookeeper_zhandle(ozookeeper_t *ozookeeper, zhandle_t **zh);

int ozookeeper_destroy(ozookeeper_t *ozookeeper);

void global_watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context);
//initialize the watcherctx object
int watcherctx_init(watcherctx_t **watcherctx,config_t *config);


#endif


