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




#ifndef _OCTOPUS_DB_BALANCE_H_
#define _OCTOPUS_DB_BALANCE_H_

#include"hash/khash.h"
#include<czmq.h>
#include"api/vertex.h"
#include"intervals.h"
#include"dbo.h"



struct db_balance_t
{

    dbo_t *dbo;
    void *router_bl;            //used to tranfer nodes to the apropriate nodes if necessary
    void *self_bl;
    char self_key[18];
    zlist_t *on_gives;          //db version
    zlist_t *don_gives;         //db version
    zlist_t *on_receives;
    zlist_t *don_receives;      //when the node goes up again, resume the transfer
    intervals_t *intervals;
    intervals_t *locked_intervals;      //those can not be transfered because they are currently loaded to the workers
    int un_id;
    int64_t next_time;
};

typedef struct db_balance_t db_balance_t;



void db_balance_init (db_balance_t ** balance, dbo_t * dbo,
                      void *router_bl, void *self_bl, char *key);

void db_balance_clear_timer (db_balance_t * balance);

//update after an event to a specific on_give
void
db_balance_update_give_timer (db_balance_t * balance, db_on_give_t * on_give);

//update after an event to a specific on_receive
void
db_balance_update_receive_timer (balance_t * balance,
                                 db_on_receive_t * on_receive);

//position the iterator to the correct position first
void
db_balance_send_next_chunk (db_balance_t * balance, db_on_give_t * on_give,
                            zframe_t * address);

void
db_balance_interval_received (db_balance_t * balance, zmsg_t * msg,
                              zframe_t * address);

void
db_balance_confirm_chunk (db_balance_t * balance, zmsg_t * msg,
                          zframe_t * address);

void
db_balance_missed_chunkes (db_balance_t * balance, zmsg_t * msg,
                           zframe_t * address);

void balance_new_chunk (balance_t * balance, zmsg_t * msg, zframe_t * address);

void db_balance_new_interval
    (db_balance_t * balance, zmsg_t * msg, zframe_t * address);

void db_balance_new_msg (db_balance_t * balance, zmsg_t * msg);

void db_balance_lazy_pirate (db_balance_t * balance);

void db_balance_init_gives (db_balance_t * balance, intervals_t * cintervals);




#endif
