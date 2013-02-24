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


#include "balance.h"

void
balance_init (balance_t ** balance, khash_t (vertices) * hash,
              void *router_bl, void *self_bl, char *self_key, void *wake_nod)
{

    *balance = malloc (sizeof (balance_t));
    (*balance)->hash = hash;
    (*balance)->router_bl = router_bl;
    (*balance)->self_bl = self_bl;
    intervals_init (&((*balance)->intervals));
    (*balance)->events = zlist_new ();
    (*balance)->actions = zlist_new ();
    (*balance)->on_gives = zlist_new ();
    (*balance)->on_receives = zlist_new ();
    (*balance)->un_id = 0;
    (*balance)->next_time = -1;
    (*balance)->pr_time = zclock_time ();
    (*balance)->self_key = self_key;
    (*balance)->wake_nod = wake_nod;


}

//update after an event to a specific on_give
void
balance_update (balance_t * balance, on_give_t * on_give)
{

    assert (on_give->state == 0 || on_give->state == 2);


    if ((on_give->last_time < balance->next_time)
        || (balance->next_time < 0)) {
        balance->next_time = on_give->last_time;
    }
}

