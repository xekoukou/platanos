#include<czmq.h>
#include"worker_update.h"

int update_init( update_t **update,void *dealer,router_t *router,balance_t *balance){
*update=(update_t *)malloc(sizeof(update_t));
(*update)->id=0;
(*update)->dealer=dealer;
(*update)->router=router;
(*update)->balance=balance;

}

int balance_init(balance_t **balance,khash_t(vertices) *hash,void *router_bl,void *self_bl,intervals_t *intervals,zlist_t *events,zlist_t *actions){

*balance=(balance_t *)malloc(sizeof(balance_t));
(*balance)->hash=hash;
(*balance)->router_bl=router_bl;
(*balance)->self_bl=self_bl;
(*balance)->intervals=intervals;
(*balance)->events=events;
(*balance)->actions=actions;

}
