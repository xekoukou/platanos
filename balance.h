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




#ifndef _OCTOPUS_BALANCE_H_
#define _OCTOPUS_BALANCE_H_

#include"hash/khash.h"
#include<czmq.h>
#include"vertex.h"
#include"intervals.h"
#include"on_give.h"
#include"on_receive.h"



struct balance_t
{

    khash_t (vertices) * hash;
    void *router_bl;            //used to tranfer nodes to the apropriate nodes if necessary
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
    char *self_key;             //used to send the interval of the on_gives
};

typedef struct balance_t balance_t;

struct on_give_t;
typedef struct on_give_t on_give_t;


void balance_init (balance_t ** balance, khash_t (vertices) * hash,
                   void *router_bl, void *self_bl, char *self_key);

//update after an event to a specific on_give
void balance_update_give_timer (balance_t * balance, on_give_t * on_give);

void balance_new_msg (balance_t * balance, zmsg_t * msg);

void balance_lazy_pirate (balance_t * balance);


#endif
