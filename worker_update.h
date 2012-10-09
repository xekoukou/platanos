#ifndef _OCTOPUS_WORKER_UPDATE_H_
#define _OCTOPUS_WORKER_UPDATE_H_
#include<czmq.h>

typedef struct {
zloop_t *loop;
unsigned int id;  //the id of the previous update
void * dealer;  //used to confirm the updates to the ozookeeper object
router_t *router;
void *balance; //used to tranfer nodes to the apropriate nodes if necessary
} updater_t;

int updater_init( updater_t **updater,zloop_t *loop,void *dealer,router_t *router,void *balance);

#endif

