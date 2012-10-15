#include<czmq.h>
#include"worker.h"
#include"worker_update.h"
#include"zookeeper.h"
#include<zookeeper/zookeeper.h>
#include"hash/khash.h"
#include"vertex.h"
#include"aknowledgements.h"

KHASH_MAP_INIT_INT64(vertices, vertex_t *);

int send( zmsg_t *msg ,router_t *router,void *socket_wb,void *socket_nb,void *self_wb,void *self_nb){
     zframe_t *frame= zmsg_first(msg);
       frame=zframe_next(msg);
      uint64_t key;
      memcpy(&key,zframe_data(frame),sizeof(uint64_t));
      char id[1000];
      router_route(router,key,id);

      //check that self has been initialized
      assert(router->self!=NULL);

      //send to self if necessary
      if(strcmp(router->self->key,id)==0){
      if(wb){
zmsg_send(&msg,self_wb);
}else{
zmsg_send(&msg,self_nb);
}

}else{
      //write address
      zmsg_wrap(msg,zframe_new(id,strlen(id)));
      if(wb){
zmsg_send(&msg,socket_wb);
}else{
zmsg_send(&msg,socket_nb);
}

}



}

int
compute (zloop_t * loop, zmq_pollitem_t * item, void *arg)
{


return 0;
}


int
worker_update (updater_t *updater,void *sub)
{

//check if it is a new update or an old one
zmsg_t *msg=zmsg_recv(sub);
zframe_t *frame=zframe_pop(msg);
if(memcmp(zframe_data(frame),&(updater->id),sizeof(unsigned int))==0){
//lazy pirate reconfirm update
zframe_send(frame,updater->dealer,0);
}else{





zframe_send(frame,updater->dealer,0);
}

return 0;
}


int sleep_send(sleep_t *sleep,router_t *router,void *socket_wb,void *socket_nb,void *self_wb,void *self_nb){
      zmsg_t *msg;
      unsigned short wb;

      while(msg=sleep_awake(sleep,&wb)){
      send(msg,router,socket_wb,socket_nb,self_wb,self_nb);

      }


}



void
worker_fn (void *arg, zctx_t * ctx, void *pipe)
{
int rc;
//update infrastructure
void *sub=zsocket_new(ctx,ZMQ_SUB);
void *dealer=zsocket_new(ctx,ZMQ_DEALER);


zmq_setsockopt (dealer,ZMQ_IDENTITY,arg, strlen(arg));
zmq_setsockopt (sub,ZMQ_SUBSCRIBE,arg, strlen(arg));
zmq_setsockopt (sub,ZMQ_SUBSCRIBE,"all",strlen("all")+1);


//worker infrastruct

void *socket_wb=zsocket_new(ctx,ZMQ_ROUTER);
void *socket_nb=zsocket_new(ctx,ZMQ_ROUTER);
void *self_wb=zsocket_new(ctx,ZMQ_DEALER);
void *self_nb=zsocket_new(ctx,ZMQ_DEALER);

char *identity=(char *)malloc(1000);

sprintf(identity,"%swb",arg);
zmq_setsockopt (self_wb,ZMQ_IDENTITY,identity, strlen(identity));
sprintf(identity,"%snb",arg);
zmq_setsockopt (self_nb,ZMQ_IDENTITY,identity, strlen(identity));

//balance infrastructure

void *balance=zsocket_new(ctx,ZMQ_ROUTER);
void *self_bl=zsocket_new(ctx,ZMQ_DEALER);
zmq_setsockopt (self_bl,ZMQ_IDENTITY,arg, strlen(arg));

//cleaning
free(identity);

//hash of vertices
khash_t(vertices) *hash = kh_init(vertices);

//sleep object
sleep_t *sleep;
sleep_init(&sleep);

//router object
//used to find where each msg goes
router_t *router;

router_init(&router,0);

//intervals object

intervals_t *intervals;

intervals_init(&intervals);

//events ,actions objects

zlist_t *actions=zlist_new();
zlist_t *events=zlist_new();


//balance object
balance_t *balance;

balance_init(balance_t **balance,hash,router_bl,self_bl,intervals,events,actions)


//update object
//used to update things, like the router object
update_t *update;


update_init(&update,dealer,router,balance);
 
zmq_pollitem_t pollitems[4] = {{ sub, 0, ZMQ_POLLIN },{ balance, 0, ZMQ_POLLIN },{ router_wb, 0, ZMQ_POLLIN },{ router_nb, 0, ZMQ_POLLIN }};
//main loop
  while (1)
    {
      rc = zmq_poll (pollitems, 4, sleep->timeout);
      assert (rc != -1);
      
//sends all msgs that their delay has expired
      sleep_send(sleep,router,socket_wb,socket_nb,self_wb,self_nb);

      if (pollitem[0].revents & ZMQ_POLLIN)
        {
        worker_update(update,sub);
}
      if (pollitem[1].revents & ZMQ_POLLIN)
        {

}
      if (pollitem[2].revents & ZMQ_POLLIN)
        {

}
      if (pollitem[3].revents & ZMQ_POLLIN)
        {

}




}





}

int workers_init(workers_t **workers,ozookeeper_t *ozookeeper){


char path[1000];
char octopus[1000];
char comp_name[1000];

oconfig_octopus(ozookeeper->config,octopus);
oconfig_comp_name(ozookeeper->config,comp_name);

sprintf(path,"/%s/%s/worker_nodes",octopus,comp_name);
struct String_vector worker_children;
result=zoo_get_children(zh,path,0,&worker_children);


if(ZOK==result){

//mallocing
*workers=(workers_t *)malloc(sizeof(worker_t));
(*workers)->size=worker_children.count;
(*workers)->pipe=(void **)malloc(sizeof(void *)*(worker_children.count));
(*workers)->id=(char **)malloc(sizeof(char *)*(worker_children.count));
//create the threads

int iter;
if(worker_children.count<1000){
for(iter=0; iter<worker_children.count; iter++){
(*workers)->id[iter]=(char *)malloc(strlen(worker_children.data[iter])+1+strlen(comp_name));
sprintf((*workers)->id[iter],"%s%s",comp_name,worker_children.data[iter]);
(*workers)->pipe[iter]=zthread_fork (ctx, &worker_fn,(*workers)->id[iter]);
}
}else{
printf("\n More workers than allowed.. error exiting");
exit(1);
}
if(ZOK!=result && ZNONODE!=result){
printf("\n Couldnt get the children.. error exiting");
exit(1);
}

}




deallocate_String_vector(worker_children);
}
