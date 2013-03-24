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


#include "db_update.h"

void
db_update_init (db_update_t ** update, void *dealer, router_t * db_router,
                balance_t * balance, db_t * db, void *in, void *out)
{
    *update = malloc (sizeof (update_t));
    (*update)->id = 0;
    (*update)->dealer = dealer;
    (*update)->balance = balance;
    (*update)->db = db;
    (*update)->db_router = db_router;
    (*update)->in = in;
    (*update)->out = out;
}
