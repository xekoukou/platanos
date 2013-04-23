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


#ifndef _OCTOPUS_DB_ON_RECEIVE_H_
#define _OCTOPUS_DB_ON_RECEIVE_H_

#include"db_balance.h"

struct db_balance_t;
typedef struct db_balance_t db_balance_t;

struct node_t;
typedef struct node_t node_t;

typedef struct
{
    char key[18];
    zlist_t *m_counters;        // counters that were sent but lost in the communication
//they are ordered
    int un_id;                  //set by the giver to distinguish the confirmation msgs
    uint64_t counter;           //counter of the last received data
    uint64_t last_time;         //used to indicate the last time it requested for the missed chunkes 
    interval_t *interval;       //at the end of the receive it is added to the rest of the intervals
} db_on_receive_t;              //ongoing event

void db_on_receive_init (db_on_receive_t ** on_receive, int id, char *key,
                         zmsg_t * msg);

void db_on_receives_destroy (db_balance_t * balance, node_t * node);

//doesnt not destroy the interval
void db_on_receive_destroy (db_on_receive_t ** on_receive);

//on_receive events are not unique per id
db_on_receive_t *on_receives_search (zlist_t * on_receives, int id, char *key);


#endif
