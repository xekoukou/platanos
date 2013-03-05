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


#ifndef _OCTOPUS_ON_RECEIVE_H_
#define _OCTOPUS_ON_RECEIVE_H_

#include"balance.h"
#include"events.h"

struct balance_t;
typedef struct balance_t balance_t;

typedef struct
{
    action_t *action;           //this action is happening, you ll need to add it to the list when it finishes
    zlist_t *m_counters;        // counters that were sent but lost in the communication
//they are ordered
    int un_id;                  //set by the giver to distinguish the confirmation msgs
    uint64_t counter;           //counter of the last received data
    uint64_t last_time;    //used to indicate the last time it requested for the missed chunkes 
} on_receive_t;                 //ongoing event

void on_receive_init (on_receive_t ** on_receive,int id,char *key, zmsg_t * msg);

//used when we receive a remove_node event
void
on_receives_destroy (zlist_t * on_receives, balance_t * balance, node_t * node);


//destroy this after you have inserted the action to the actions list
//or removed the corresponding event
void on_receive_destroy (on_receive_t * on_receive);

//on_receive events are not unique per id
on_receive_t * on_receives_search(zlist_t *on_receives,int id,char *key);


#endif
