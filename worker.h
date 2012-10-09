#include<czmq.h>

#ifndef OCTOPUS_WORKER_H_
#define OCTOPUS_WORKER_H_


//arg is a const integer  
//great care not to change that integer
//it is used in the sub and dealer socket (subscription, identity)
//max 1000 workers per computer
//worker ids are from 0 till size-1
void worker_fn (void *arg, zctx_t * ctx, void *pipe);

typedef struct {
 void **pipe;
 char **id; //has null at the end
 int size;
} workers_t;



int workers_init(workers_t **workers,ozookeeper_t *ozookeeper);

#endif
