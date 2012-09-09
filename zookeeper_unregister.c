#include<zookeeper/zookeeper.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include"config.h"


#define _LL_CAST_ (long long)


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


    fprintf(stderr, "Watcher %s state = %s", type2String(type), state2String(state));
    if (path && strlen(path) > 0) {
      fprintf(stderr, " for path %s", path);
    }
    fprintf(stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
                fprintf(stderr, "Reconnected with session id: 0x%llx\n",
                        _LL_CAST_ ((zoo_client_id(zzh))->client_id));
            }
        } else {
                   if (state == ZOO_AUTH_FAILED_STATE) {
                   fprintf(stderr, "Authentication failure. Shutting Down...\n");
                   zookeeper_close(zzh);

                } else if (state == ZOO_EXPIRED_SESSION_STATE) {
                   fprintf(stderr, "Session expired. Shutting Down...\n");
                   zookeeper_close(zzh);

           }
     }
}



//strings count data

int main(){

printf("\nI am going to unregister the resource that is described in the config file and then delete the config file. Are you sure you want to continue?(1 or 0)");

int sure;

scanf("%d",&sure);

if(sure!=1){
return 0;
}


oconfig_t *config;
oconfig_init(&config);

int result;
char host[1000];
char path[1000];

oconfig_host(config,host);


zhandle_t *zh=zookeeper_init(host,global_watcher, 3000, 0,0,0);
char comp_name[1000];
oconfig_comp_name(config,comp_name);

sprintf(path,"/%s",comp_name);
struct String_vector children;
result=zoo_get_children(zh,path,0,&children);
if(ZOK!=result){
printf("\n Couldnt get the children.. exiting");
return 1;
}

char res_name[1000];
oconfig_res_name(config,res_name);

sprintf(path,"/%s/%s/n_pieces",comp_name,res_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}

sprintf(path,"/%s/%s/port",comp_name,res_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}

sprintf(path,"/%s/%s",comp_name,res_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}

if(children.count==2){
printf("\nThere are no more resources registered in this computer, deleting the computer node as well");

sprintf(path,"/%s/resources/max_memory",comp_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}

sprintf(path,"/%s/resources/free_memory",comp_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}

sprintf(path,"/%s/resources",comp_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}



sprintf(path,"/%s",comp_name);
result=zoo_delete(zh,path,-1);
if(ZOK!=result && ZOK!=ZNONODE){
printf("\n Error.. exiting");
return 1;
}


}


result=remove("./config");

if(result){
printf("\nCouldnt delete config, delete it manually");
}


}
