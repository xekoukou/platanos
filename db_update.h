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




#ifndef _OCTOPUS_DB_UPDATE_H_
#define _OCTOPUS_DB_UPDATE_H_

#include"db_balance.h"
#include"db.h"
#include"router.h"

struct db_t;
typedef struct db_t db_t;


struct db_update_t
{
    unsigned int id;            //the id of the previous update
    void *dealer;               //used to confirm the updates to the ozookeeper object
    db_balance_t *balance;
    db_t *db;
    void *in;
    void *out;
    router_t *db_router;
};

typedef struct db_update_t db_update_t;

void db_update_init (db_update_t ** update, void *dealer, router_t * db_router,
                     db_balance_t * balance, db_t * db, void *in, void *out);




#endif
