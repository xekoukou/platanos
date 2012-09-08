#include<zookeeper/zookeeper.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>


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

int main(){

//backup previous config

printf("Starting Registering Process");

printf("\n Backing up previous config file");

int iter=0;
char location[100];
FILE *fconfig;
if(fconfig=fopen("./config","r")){
iter=1;
}
while(fconfig){
sprintf(location,"./config%d",iter);
fconfig=fopen(location,"r");
iter++;
}

if(iter){
assert(0==rename("./config",location));
}

char config[6][1000];

printf("\nPlease, provide the connecting points (ip:port,ip:port) to the zookeeper ensemple");

scanf("%s",config[0]);

printf("\n recv_delay:");
scanf("%s",config[1]);

printf("A unique computer name, common across all resources and configs of this computer:");

scanf("%s",config[2]);

printf("\nIs this computer name new or has it already been initialized by other resources? 1 or 0)");
int new_computer;

scanf("%d",new_computer);

printf("A unique to this computer name for the resource:");

scanf("%s",config[3]);

printf("The number of 'chunks' in the hash ring that will be this resources responsibility:");

scanf("%s",config[4]);

//ip could be dynamic, or the computer might change location
printf("\nThe port to connect to:");

scanf("%s",config[5]);

zhandle_t *zh=zookeeper_init(config[0],global_watcher, 3000, 0,0,0);

int result;
char path[1000];
sprintf(path,"/%s",config[2]);
result=zoo_create(zh,path,NULL,-1,&ZOO_READ_ACL_UNSAFE,0,NULL,0);

if(result==ZNODEEXISTS && new_computer){

printf("This computer name already exists, exiting..");
return 0;
}


if(result==ZOK && new_computer==0){
printf("this computer_name doesnt already exist, reverting back and exiting..");
if(ZOK==zoo_delete(zh,config[2],1)){
return 0;
}else {printf("\nThere has been an error"); 
       return 1;
       }         
}
// the resource name

sprintf(path,"/%s/%s",config[2],config[3]);
result=zoo_create(zh,path,NULL,-1,&ZOO_READ_ACL_UNSAFE,0,NULL,0);

if(ZNODEEXISTS==result){
printf("\nThis resource name has already been assigned to a resource of this computer, exiting..."); 
return 0;
} else{
       if(ZOK==result){
                       sprintf(path,"/%s/%s/n_pieces",config[2],config[3]);
                       result=zoo_create(zh,path,config[4],strlen(config[4]),&ZOO_READ_ACL_UNSAFE,0,NULL,0);
       if(ZOK==result){
                       sprintf(path,"/%s/%s/port",config[2],config[3]);
                       result=zoo_create(zh,path,config[5],strlen(config[5]),&ZOO_READ_ACL_UNSAFE,0,NULL,0);
                     }
                     }

}

printf("\nAll have gone smoothly, saving configuration and exiting");
fconfig=fopen("./config","w");

for(iter=0; iter<6; iter++){
fprintf(fconfig,"%s\n",config[iter]);
}
fclose(fconfig);

}
