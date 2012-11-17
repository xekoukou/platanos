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





#include<czmq.h>
#include"worker_update.h"
#include"vertex.h"
#include"hash/khash.h"
#include"aknowledgements.h"
#include<stdlib.h>

//TODO this is arbitrary
#define ONGOING_TIMEOUT 10000

void
update_init (update_t ** update, void *dealer, router_t * router,
	     balance_t * balance, compute_t * compute)
{
    *update = malloc (sizeof (update_t));
    (*update)->id = 0;
    (*update)->dealer = dealer;
    (*update)->router = router;
    (*update)->balance = balance;
    (*update)->compute = compute;
}

void
on_give_init (on_give_t ** on_give, event_t * event, int un_id)
{
    *on_give = malloc (sizeof (on_give_t));
    (*on_give)->event = event;
    (*on_give)->unc_vertices = zlist_new ();
    (*on_give)->rec_counter = 0;
    (*on_give)->un_id = un_id;
    (*on_give)->last_time = zclock_time ();
    (*on_give)->state = 0;
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
