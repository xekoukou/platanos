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


#include "on_receive.h"


void
on_receive_init (on_receive_t ** on_receive, zmsg_t * msg)
{
    *on_receive = malloc (sizeof (on_receive_t));

    zframe_t *frame = zmsg_first (msg);
    memcpy (&((*on_receive)->un_id), zframe_data (frame), sizeof (int));

    action_minit (&((*on_receive)->action), msg);

    (*on_receive)->m_counters = zlist_new ();
    (*on_receive)->counter = 0;

}


//destroy this after you have inserted the action to the actions list
//or removed the corresponding event
void
on_receive_destroy (on_receive_t * on_receive)
{
    assert (on_receive->m_counters != NULL);
    zlist_destroy (&(on_receive->m_counters));
    free (on_receive);
}



void
on_receives_destroy (zlist_t * on_receives, balance_t * balance, node_t * node)
{

    on_receive_t *iter = zlist_first (on_receives);
    while (iter) {
        if (strcmp (node->key, iter->action->key) == 0) {


//erase event if it exists
            if (events_update (balance->events, iter->action)) {
                free (iter->action);
            }
            else {
//add action to the list
                zlist_append (balance->actions, iter->action);

            }
//check whether any of the events can occur due to this action

            event_t *event;
            while ((event =
                    events_possible (balance->events, balance->intervals))) {
//perform this event

//update the intervals

                interval_t *interval;
                interval_init (&interval, &(event->start), &(event->end));
                intervals_remove (balance->intervals, interval);

//update un_id;
                if (balance->un_id > 1000000000) {
                    balance->un_id = 1;
                }
                else {
                    balance->un_id++;
                }


//create on_give object
                on_give_t *on_give;
                on_give_init (&on_give, event, balance->un_id);

//update balance object
                balance_update (balance, on_give);


//put on_give event into the list
                zlist_append (balance->on_gives, on_give);
            }


            zlist_remove (balance->on_receives, iter);
            on_receive_destroy (iter);
        }

        iter = zlist_next (on_receives);
    }

}
