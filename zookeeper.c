#include<zookeeper/zookeeper.h>
#include"config.h"
#include"zookeeper.h"
#include<string.h>


int global_watcherctx_init(global_watcherctx_t **watcherctx,oconfig_t *config){
*watcherctx=(global_watcherctx_t *)malloc(sizeof(global_watcherctx_t));
(*watcherctx)->retries=0;
(*watcherctx)->max_retries=2000;
(*watcherctx)->config=config;
}

//initialize the ozookeeper object
int ozookeeper_init(ozookeeper_t **ozookeeper, oconfig_t *config,global_watcherctx_t *watcherctx){

*ozookeeper=(ozookeeper_t *)malloc(sizeof(ozookeeper_t));

(*ozookeeper)->config=config;

char host[1000];
oconfig_host(config,host);
int recv_timeout;
oconfig_recv_timeout(config,&recv_timeout);

watcherctx->ozookeeper=*ozookeeper;

(*ozookeeper)->zh=zookeeper_init(host, global_watcher, recv_timeout, 0,watcherctx,0);

if(ozookeeper_not_corrupt(ozookeeper)!=1){
printf("\n local config and zookeeper config do not match, exiting..");
exit(1);
}



}


// 1 for true 0 for false
//checks that the node described in the config exists in the zookeeper
//if not then the config or the zookeeper are corrupt.
int ozookeeper_not_corrupt(ozookeeper_t **ozookeep){

ozookeeper_t *ozookeeper=*ozookeep;

struct Stat stat;
char path[1000];
char config[4][1000];
int db_bool;
int val_len;

oconfig_octopus(ozookeeper->config,config[0]);
oconfig_comp_name(ozookeeper->config,config[1]);
oconfig_res_name(ozookeeper->config,config[3]);

sprintf(path,"/%s",config[0]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}
sprintf(path,"/%s/%s",config[0],config[1]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}

if(oconfig_db_node(ozookeeper->config)){
db_bool=1;
sprintf(config[2],"db_nodes");
} else if (oconfig_worker_node(ozookeeper->config)){
db_bool=0;
sprintf(config[2],"worker_nodes");
}else {
return 0;
}

sprintf(path,"/%s/%s/%s/%s",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}

int zn_pieces=-1;
sprintf(path,"/%s/%s/%s/%s/n_pieces",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_get(ozookeeper->zh,path,0,(char *)&zn_pieces,&val_len,&stat)){
return 0;
} else {
int n_pieces;
oconfig_n_pieces(ozookeeper->config,&n_pieces);
if(zn_pieces!=n_pieces){
return 0;
}
}

sprintf(path,"/%s/%s/%s/%s/st_peice",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}

char zbind_point[1000];
sprintf(path,"/%s/%s/%s/%s/bind_point",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_get(ozookeeper->zh,path,0,(char *)&zbind_point,&val_len,&stat)){
return 0;
} else {
char bind_point[1000];
oconfig_bind_point(ozookeeper->config,bind_point);
if(strcmp(zbind_point,bind_point)!=0){
return 0;
}
}

if(db_bool){


char zdb_point[1000];
sprintf(path,"/%s/%s/%s/%s/db_point",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_get(ozookeeper->zh,path,0,(char *)&zdb_point,&val_len,&stat)){
return 0;
} else {
char db_point[1000];
oconfig_bind_point(ozookeeper->config,db_point);
if(strcmp(zdb_point,db_point)!=0){
return 0;
}
}



}else {

printf(path,"/%s/%s/%s/%s/interval",config[0],config[1],config[2],config[3]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}
}


return 1;
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
char octopus[1000];

oconfig_comp_name(ozookeeper->config,comp_name);
oconfig_res_name(ozookeeper->config,res_name);
oconfig_octopus(ozookeeper->config,octopus);

char type[1000];

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

global_watcherctx_t *watcherctx= (global_watcherctx_t*) context;

    fprintf(stderr, "Watcher %s state = %s", type2String(type), state2String(state));
    if (path && strlen(path) > 0) {
      fprintf(stderr, " for path %s", path);
    }
    fprintf(stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
                watcherctx->retries=0;
                fprintf(stderr, "(Re)connected with session id: 0x%llx\n",
                        _LL_CAST_ (zoo_client_id(zzh))->client_id);
            }
        } else {if (watcherctx->retries<watcherctx->max_retries){
                   if (state == ZOO_AUTH_FAILED_STATE) {
                   fprintf(stderr, "Authentication failure. Retrying...\n");
                   watcherctx->retries++;
                   ozookeeper_destroy(watcherctx->ozookeeper);
                   ozookeeper_init(&(watcherctx->ozookeeper),watcherctx->config,watcherctx);

                } else if (state == ZOO_EXPIRED_SESSION_STATE) {
                   fprintf(stderr, "Session expired. Retrying...\n");
                   watcherctx->retries++;
                   ozookeeper_destroy(watcherctx->ozookeeper);
                   ozookeeper_init(&(watcherctx->ozookeeper),watcherctx->config,watcherctx);

           }
           } else {
              fprintf(stderr, "Maximum retries reached. Shutting down...\n");
              ozookeeper_destroy(watcherctx->ozookeeper);
              global_watcherctx_destroy(watcherctx); 
                 }
          }
   }

