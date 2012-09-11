#include<zookeeper.h>
#include"config.h"
#include"zookeeper.h"

int global_watcherctx_init(globa_watcherctx_t **watcherctx,config_t *config){
*watcherctx=(global_watcherctx_t *)malloc(sizeof(global_watcherctx_t));
watcherctx->replies=0;
watcherctx->max_retries=2000;
watcherctx->config=config;
}

int ozookeeper_init(ozookeeper_t **ozookeeper, config_t *config,global_watcherctx_t *watcherctx){

*ozookeeper=(ozookeeper_t *)malloc(sizeof(ozookeeper_t));

*ozookeeper->config=config;

char host[1000];
oconfig_host(config,host);
int recv_timeout;
oconfig_recv_timeout(config,&recv_timeout);

watcherctx->ozookeeper=*ozookeeper;

ozookeeper->zh=zookeeper_init(host, global_watcher, recv_timeout, 0,0,watcherctx);

//check that the computer name and hr_name are unique
//then update/create the node that holds the configuration

}

int ozookeeper_set_zhandle(ozookeeper_t *ozookeeper, zhandle_t *zh){
ozookeeper->zh=zh;
}
int ozookeeper_zhandle(ozookeeper_t *ozookeeper, zhandle_t **zh){
*zh=ozookeeper->zh;
}


int ozookeeper_destroy(ozookeeper_t *ozookeeper){
zookeeper_close(ozookeeper->zh);
free(ozookeeper);
}

int global_watcherctx_destroy(global_watcherctx_t *watcherctx){
free(watcherctx);
}

//creating ephemeral node online
int ozookeeper_online(ozookeeper_t *ozookeeper){
char comp_name[1000];
char res_name[1000];

oconfig_comp_name(ozookeeper->config,comp_name);
oconfig_res_name(ozookeeper->config,res_name);

int result;
char path[1000];
sprintf(path,"/%s/%s/online",comp_name,res_name);
result=zoo_create(zh,path,NULL,-1,&ZOO_OPEN_ACL_UNSAFE,ZOO_EPHEMERAL,NULL,0);

if(result!=ZNODEEXISTS && result!=ZOK ){
printf("\nCouldnt create ephemeral node online, exiting");
}

}

//one time function that sets watches on the nodes
int ozookeeper_watch(ozookeeper_t *ozookeeper){

}


static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

static const char* type2String(int state){
  if (state == ZOO_CREATED_EVENT)
    return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT)
    return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT)
    return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT)
    return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT)
    return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT)
    return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}


void global_watcher(zhandle_t *zzh, int type, int state, const char *path,
             void* context)
{

watcherctx_t *watcherctx= (watcherctx_t*) context;

    fprintf(stderr, "Watcher %s state = %s", type2String(type), state2String(state));
    if (path && strlen(path) > 0) {
      fprintf(stderr, " for path %s", path);
    }
    fprintf(stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
                watcherctx->retries=0;
                fprintf(stderr, "Reconnected with session id: 0x%llx\n",
                        _LL_CAST_ (zoo_client_id(zzh))->client_id);
            }
        } else {if (watcher->retries<watcher->max_retries){
                   if (state == ZOO_AUTH_FAILED_STATE) {
                   fprintf(stderr, "Authentication failure. Retrying...\n");
                   watcherctx->retries++;
                   ozookeeper_destroy(watcherctx->ozookeeper);
                   ozookeeper_init(watcherctx->ozookeeper,watcherctx->config,watcherctx);

                } else if (state == ZOO_EXPIRED_SESSION_STATE) {
                   fprintf(stderr, "Session expired. Retrying...\n");
                   watcherctx->retries++;
                   ozookeeper_destroy(watcherctx->ozookeeper);
                   ozookeeper_init(watcherctx->ozookeeper,watcherctx->config,watcherctx);

           }
           } else {
              fprintf(stderr, "Maximum retries reached. Shutting down...\n");
              ozookeeper_destroy(watcherctx->ozookeeper);
              global_watcherctx_destroy(watcherctx); 
                 }
          }
   }
}

