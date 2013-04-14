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
    intervals_t *intervals;
    intervals_t *locked_intervals;      //those can not be transfered because they are currently loaded to the workers
    int un_id;
    int64_t next_time;
};

typedef struct db_balance_t db_balance_t;



void db_balance_init (db_balance_t ** balance, dbo_t * dbo,
                      void *router_bl, void *self_bl, char *key);


#endif
