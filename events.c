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


#include "events.h"


event_t *
events_search (zlist_t * events, action_t * action)
{
    event_t *iter = zlist_first (events);
    while (iter) {
        if (cmp_ev_ac (iter, action)) {
            return iter;
        }
        iter = zlist_next (events);
    }
    return NULL;

}

event_t *
events_remove (zlist_t * events, node_t * node,int reset)
{
if(reset){
    event_t *iter = zlist_first (events);
}
    while (iter) {

        if (strcmp (node->key, iter->key) == 0) {
            zlist_remove (events, iter);
            return iter;
        }


        iter = zlist_next (events);
    }

return NULL;



}


//returns 1 if there was an event that was erased by this action
int
events_update (zlist_t * events, action_t * action)
{
    event_t *result = events_search (events, action);
    if (result) {
        zlist_remove (events, result);

        free (result);

        return 1;
    }
    return 0;


}


//returns the first event that can occur with the current nodes that we possess or NULL
event_t *
events_possible (zlist_t * events, intervals_t * intervals)
{
    interval_t *interval;
    interval_init (&interval, NULL, NULL);

    event_t *iter = zlist_first (events);
    while (iter) {
        if (iter->give) {
            memcpy (&(interval->start), &(iter->start),
                    sizeof (struct _hkey_t));
            memcpy (&(interval->end), &(iter->end), sizeof (struct _hkey_t));
            int circle;
            interval_t *is_contained =
                intervals_contained (intervals, interval, &circle);
            if (circle || is_contained) {
                return iter;
            }

        }
        iter = zlist_next (events);
    }
    free (interval);
    return NULL;

}
