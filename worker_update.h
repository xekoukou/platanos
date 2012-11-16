/*
    Copyright contributors as noted in the AUTHORS file.
                
    This file is part of PLATANOS.

    PLATANOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
            
    PLATANOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
        
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/




#ifndef _OCTOPUS_WORKER_UPDATE_H_
#define _OCTOPUS_WORKER_UPDATE_H_
#include<czmq.h>
#include"worker.h"
#include"vertex.h"
#include"hash/khash.h"



typedef struct
{
    event_t *event;		//the event that is happening, must also be removed from events list before freed
    zlist_t *unc_vertices;	//vertices that have been send but not confirmed
    // contains the khiter where it is saved by the khash

    uint64_t rec_counter;	//the counter of the last confirmation
    int un_id;
    uint64_t last_time;		//last_time till I sent the interval(initial msg) or last time till
//i sent the ending msg(0 counter)
//those operations will be repeated until they succeed
    int state;
//state 0 giving the interval
//state 1 giving the main body
//state 2 sending the ending msg
} on_give_t;			//ongoing event


typedef struct
{
    action_t *action;		//this action is happening, you ll need to add it to the list when it finishes
    zlist_t *m_counters;	// counters that were sent but lost in the communication
//they are ordered
    int un_id;			//set by the giver to distinguish the confirmation msgs
    uint64_t counter;		//counter of the last received data
} on_receive_t;			//ongoing event


typedef struct
{

    khash_t (vertices) * hash;
    void *router_bl;		//used to tranfer nodes to the apropriate nodes if necessary
    void *self_bl;
    intervals_t *intervals;
    zlist_t *events;
    zlist_t *actions;
    zlist_t *on_gives;
    zlist_t *on_receives;
    //used by on_gives scheduling
    int un_id;
    int64_t next_time;
    int64_t pr_time;
    char *self_key;		//used to send the interval of the on_gives
    void *wake_nod;		//socket to awake the worker poll
} balance_t;


typedef struct
{
    unsigned int id;		//the id of the previous update
    void *dealer;		//used to confirm the updates to the ozookeeper object
    router_t *router;
    balance_t *balance;
    compute_t *compute;
} update_t;

void update_init (update_t ** update, void *dealer, router_t * router,
		  balance_t * balance, compute_t * compute);

void balance_init (balance_t ** balance, khash_t (vertices) * hash,
		   void *router_bl, void *self_bl, char *self_key,
		   void *wake_nod);

void balance_update (balance_t * balance, on_give_t * on_give);

void on_give_init (on_give_t ** on_give, event_t * event, int un_id);

void on_give_destroy (on_give_t * on_give);

void on_receive_init (on_receive_t ** on_receive, zmsg_t * msg);

//destroy this after you have inserted the action to the actions list
//or removed the corresponding event
void on_receive_destroy (on_receive_t * on_receive);


#endif
