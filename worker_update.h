#ifndef _OCTOPUS_WORKER_UPDATE_H_
#define _OCTOPUS_WORKER_UPDATE_H_
#include<czmq.h>
#include"worker.h"

typedef struct {

khash_t(vertices) *hash;
void *router_bl; //used to tranfer nodes to the apropriate nodes if necessary
void *self_bl;
intervals_t *intervals;
zlist_t *events;
zlist_t *actions;
}balance_t;


typedef struct {
unsigned int id;  //the id of the previous update
void * dealer;  //used to confirm the updates to the ozookeeper object
router_t *router;
balance_t *balance;
} update_t;

int update_init( update_t **update,void *dealer,router_t *router,balance_t *balance);


#endif

