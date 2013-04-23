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


#include "db_on_receive.h"


void
db_on_receive_init (db_on_receive_t ** on_receive, int id, char *key, zmsg_t * msg)
{
    *on_receive = malloc (sizeof (on_receive_t));

    (*on_receive)->un_id = id;


    interval_minit (&((*on_receive)->interval), msg);

    (*on_receive)->m_counters = zlist_new ();
    (*on_receive)->counter = 0;
    (*on_receive)->last_time = 0;       //should only be checked if theu are miising chunkes

}


void
db_on_receive_destroy (db_on_receive_t ** on_receive)
{
    assert ((*on_receive)->m_counters != NULL);
    zlist_destroy (&((*on_receive)->m_counters));
    free (*on_receive);
    on_receive = NULL;
}


//on_receive events are not unique per id
db_on_receive_t *
db_on_receives_search (zlist_t * on_receives, int id, char *key)
{

    db_on_receive_t *iter = zlist_first (on_receives);
    while (iter) {
        if ((id == iter->un_id) && (memcmp (key, iter->key, 7) == 0)) {
            return iter;
        }
        iter = zlist_next (on_receives);
    }

    return NULL;

}
