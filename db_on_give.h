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


#ifndef _OCTOPUS_DB_ON_GIVE_H_
#define _OCTOPUS_DB_ON_GIVE_H_

#include"node.h"
#include"api/vertex.h"
#include "db_balance.h"

struct node_t;
typedef struct node_t node_t;



struct db_on_give_t
{
    char key[18];               //the key of the node we send to
    char unc_iter[50];          //the first key of of the iterator that points to unconfirmed keys

    uint64_t rec_counter;       //the counter of the last confirmation
    int un_id;
    uint64_t last_time;         //last_time till I sent the interval(initial msg) or last time till
//i sent the ending msg(0 counter)
//those operations will be repeated until they succeed
    int state;                  //0 for interval send
    //1 for ending msg
//last counter is used for the last chunk plus to see if I have already received an Interval_received before
//due to possible duplicates (lazy pirate)
//must be set to zero

    interval_t *interval;       //used to fetch only the keys inside the event
    zmsg_t *responce;           //the first common frames of the new_chunk responce
    int pending_confirmations;
    uint64_t last_counter;

//iterator
    leveldb_iterator_t *iter;
    leveldb_readoptions_t *readoptions;

};                              //ongoing event

typedef struct db_on_give_t db_on_give_t;

struct db_balance_t;
typedef struct db_balance_t db_balance_t;

void db_on_give_init (db_on_give_t ** on_give, db_balance_t * balance,char *key,interval_t * interval, int un_id);

void
db_on_give_reset (db_on_give_t * on_give);

void db_on_give_destroy (on_give_t ** on_give,db_balance_t *balance);

void db_on_gives_dead (db_balance_t * balance, node_t * node);
//linear search
db_on_give_t *db_on_gives_search_id (zlist_t * on_gives, int id);

#endif
