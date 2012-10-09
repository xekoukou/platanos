#include<czmq.h>

int updater_init( updater_t **updater,zloop_t *loop,void *dealer,router_t *router,void *balance){
*updater=(updater_t *)malloc(sizeof(updater_t));
(*updater)->id=0;
(*updater)->loop=loop;
(*updater)->dealer=dealer;
(*updater)->router=router;
(*updater)->balance=balance;

}
