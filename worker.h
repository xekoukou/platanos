#include<czmq.h>
#include"hash/khash.h"
#include"btree/kbtree.h"
#include"localdb.h"
#include"vertex.h"

#ifndef OCTOPUS_WORKER_H_
#define OCTOPUS_WORKER_H_


typedef struct
{
    zhandle_t *zh;
    oconfig_t *config;
    char *id;
} worker_t;

int worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config,
		 char *id);



typedef struct
{
    kbtree_t (vertices) * tree;
    void *socket_nb;
    void *self_nb;
    void *socket_wb;
    void *self_wb;
    localdb_t *localdb;
    unsigned long counter;
    int interval;		//the interval in which the counter resides
    unsigned long interval_size;
    worker_t *worker;		//this is the object given to the thread, it also has
} compute_t;			//the zookeeper handle used to set the worker online or get
		   //get the interval 

int compute_init (compute_t ** compute, kbtree_t (vertices) * tree,
		  void *socket_nb, void *self_nb, void *socket_wb,
		  void *self_wb, localdb_t * localdb, worker_t * worker);



//arg is a const integer  
//great care not to change that integer
//it is used in the sub and dealer socket (subscription, identity)
//max 1000 workers per computer
//worker ids are from 0 till size-1
void worker_fn (void *arg, zctx_t * ctx, void *pipe);

typedef struct
{
    void **pipe;
    char **id;			//has null at the end
    int size;
} workers_t;



int workers_init (workers_t ** workers, ozookeeper_t * ozookeeper);

#endif
