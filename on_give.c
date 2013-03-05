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


#include "on_give.h"


void
on_give_init (on_give_t ** on_give, event_t * event, int un_id)
{
    *on_give = malloc (sizeof (on_give_t));
    (*on_give)->event = event;
    (*on_give)->unc_vertices = zlist_new ();
    (*on_give)->rec_counter = 0;
    (*on_give)->last_counter = 0;
    (*on_give)->state = 0;
    (*on_give)->un_id = un_id;
    (*on_give)->last_time = zclock_time ();
}


//destroy this after you have removed the event from the events list
//this will free the event
//also all the vertices need to be freed before destroying this
void
on_give_destroy (on_give_t * on_give)
{
    free (on_give->event);
    assert (on_give->unc_vertices != NULL);
    zlist_destroy (&(on_give->unc_vertices));
    free (on_give);
}


//we assume that all vertices of this transaction have
//been put in the unc_vertices list
void
on_gives_remove (zlist_t * on_gives, zlist_t * events, node_t * node)
{

    on_give_t *iter = zlist_first (on_gives);

    while (iter) {
        if (strcmp (node->key, iter->event->key) == 0) {
            zlist_remove (on_gives, iter);
            zlist_remove (events, iter->event);
            free (iter->event);
            vertex_t *vertex = zlist_pop (iter->unc_vertices);
            while (vertex) {
                free (vertex);
                vertex = zlist_pop (iter->unc_vertices);
            }
            zlist_destroy (&(iter->unc_vertices));
            free (iter);
        }
        iter = zlist_next (on_gives);
    }

}

//linear search
//since you give the id, it is unique, thus no need to search the key as well
on_give_t *
on_gives_search_id (zlist_t * on_gives, int id)
{

    on_give_t *iter = zlist_first (on_gives);
    while (iter) {
        if (id == iter->un_id) {
            return iter;
        }
    }

    return NULL;
}
