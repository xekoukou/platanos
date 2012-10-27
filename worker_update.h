#ifndef _OCTOPUS_WORKER_UPDATE_H_
#define _OCTOPUS_WORKER_UPDATE_H_
#include<czmq.h>
#include"worker.h"
#include"vertex.h"
#include"hash/khash.h"
#include"btree/kbtree.h"



typedef struct
{
    event_t *event;		//the event that is happening, must also be removed from events list before freed
      kbtree_t (vertices) * unc_vertices;	//vertices that have been send but not confirmed
    uint64_t counter;		//the counter of the last sent chuck of data
    uint64_t rec_counter;	//the counter of the last confirmation
    int un_id;
    uint64_t last_time;		//last_time till I sent the interval(initial msg) or last time till
//i sent the ending msg(0 counter)
//those operations will be repeated until they succeed
} on_give_t;			//ongoing event


KBTREE_INIT (counters, uint64_t, kb_generic_cmp);
typedef struct
{
    action_t *action;		//this action is happening, you ll need to add it to the list when it finishes
      kbtree_t (counters) * m_counters;	// counters that were sent but lost in the communication
    int un_id;			//set by the giver to distinguish the confirmation msgs
} on_receive_t;			//ongoing event


typedef struct
{

    kbtree_t (vertices) * tree;
    void *router_bl;		//used to tranfer nodes to the apropriate nodes if necessary
    void *self_bl;
    intervals_t *intervals;
    zlist_t *events;
    zlist_t *actions;
    zlist_t *on_gives;
    zlist_t *on_recieves;
    int un_id;
} balance_t;


typedef struct
{
    unsigned int id;		//the id of the previous update
    void *dealer;		//used to confirm the updates to the ozookeeper object
    router_t *router;
    balance_t *balance;
    compute_t *compute;
} update_t;

int update_init (update_t ** update, void *dealer, router_t * router,
		 balance_t * balance, compute_t * compute);

int balance_init (balance_t ** balance, kbtree_t (vertices) * tree,
		  void *router_bl, void *self_bl, intervals_t * intervals,
		  zlist_t * events, zlist_t * actions);

int on_give_init (on_give_t ** on_give, event_t * event);

int on_give_destroy (on_give_t * on_give);

int on_receive_init (on_receive_t ** on_receive, zmsg_t * msg);

//destroy this after you have inserted the action to the actions list
//or removed the corresponding event
int on_receive_destroy (on_receive_t * on_receive);


#endif
