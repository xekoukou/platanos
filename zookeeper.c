#include<zookeeper/zookeeper.h>
#include"config.h"
#include"zookeeper.h"
#include<string.h>

#define RESEND_INTERVAL 1000

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
(*ozookeeper)->pub=pub;
(*ozookeeper)->router=router;
(*ozookeeper)->id=0;


char host[1000];
oconfig_host(config,host);
int recv_timeout;
oconfig_recv_timeout(config,&recv_timeout);

watcherctx->ozookeeper=*ozookeeper;

(*ozookeeper)->zh=zookeeper_init(host, global_watcher, recv_timeout, 0,watcherctx,0);

if(ozookeeper_not_corrupt(ozookeeper)!=1){
printf("\n local config and zookeeper config do not match,\n Have you registered at least one resource for this computer to zookeeper?\n exiting..");
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

sprintf(path,"/%s",config[0]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}
sprintf(path,"/%s/%s",config[0],config[1]);
if(ZOK!=zoo_exists(ozookeeper->zh,path,0,&stat)){
return 0;
}

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

int ozookeeper_update(ozookeeper_t *ozookeeper,zmsg_t **msg){
  
if(ozookeeper->id>1000000000){
ozookeeper->id=1;
}else {
ozookeeper->id++;
};
 
  zframe_t *address;
  zframe_t *frame;

  zmsg_t *msg_to_send=zmsg_dup(*msg);
  zmsg_push(msg_to_send,zframe_new(&(ozookeeper->id),sizeof(unsigned int)));
  zmsg_push(msg_to_send,zframe_new("all",strlen("all")+1));

  zmsg_send (msg_to_send, ozookeeper->pub);

  //initializes ok vector
  zlist_t *ok_list=zlist_new();

  zmsg_t *resp;
  size_t time=zclock_time();
  while (1)
    {
      int new=1;
      resp = zmsg_recv (ozookeeper->router);
      address = zmsg_unwrap (resp);
      frame = zmsg_first (resp);
      //if confirm exists and address is new the ok vector is updated
       if(memcmp(zframe_data(frame),ozookeeper->id,sizeof(unsigned int))==0){ 
             zframe_t *iter=zlist_first(ok_list);
             while(iter){
              if (zframe_eq (iter, address))
                {
                  new=0;
                  break;
                }
                iter=zlist_next(ok_list);
                }


              if (new)
                {
                  zlist_append(ok_list, address);
                } else{
                  zframe_destroy(&address);
}
} else{

zframe_destroy(&address);

}
                        
zmsg_destroy(&resp);



if(zlist_size(ok_list)==workers->size){
//destroy things
 iter=zlist_first(ok_list);
 while(iter){
 zframe_destroy(&iter);
 iter=zlist_next(ok_list);
}
break;
}
//send again to those that didnt respond
//it assumes that the address of the dealer is the same with
//the subscription of the sub.
if(zclock_time()-time>RESEND_INTERVAL){
time=zclock_time();

int it;
for(it=0; it<workers->size; it++){
int exists=0;
iter=zlist_first(ok_list);
while(iter){
if(memcmp(zframe_data(address),&it,sizeof(int))==0){
exists=1;
break;
}
iter=zlist_next(ok_list);
}
if(exists==0){
  msg_to_send=zmsg_dup(*msg);
  zmsg_push(msg_to_send,zframe_new(&(ozookeeper->id),sizeof(unsigned int)));
  zmsg_push(msg_to_send,zframe_new(&it,sizeof(int)));
  zmsg_send (msg_to_send, ozookeeper->pub);

}
}

}

}
}
//fetches the configuration from the server and informs the threads
int ozookeeper_init_workers(ozookeeper_t *ozookeeper, workers_t *workers){
ozookeeper->workers=workers;
}


//one time function that sets watches on the nodes
//and gets configuration
int ozookeeper_getconfig(ozookeeper_t *ozookeeper){

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

