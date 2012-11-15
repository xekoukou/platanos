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
#include"worker.h"
#include"worker_update.h"
#include"zookeeper.h"
#include<zookeeper/zookeeper.h>
#include"hash/khash.h"
#include"vertex.h"
#include"aknowledgements.h"
#include"sleep.h"
#include<stdlib.h>

//TODO this is arbitrary
#define ONGOING_TIMEOUT 10000
#define COUNTER_SIZE 1000	/* 1000 vertices per chunk */

#define NEW_INTERVAL "\001"
#define NEW_CHUNK    "\002"
#define INTERVAL_RECEIVED "\003"
#define CONFIRM_CHUNK    "\004"
#define MISSED_CHUNKES    "\005"

void
worker_send (zmsg_t * msg, unsigned short wb, compute_t * compute)
{
    zframe_t *frame = zmsg_first (msg);
    frame = zmsg_next (msg);
    uint64_t key;
    memcpy (&key, zframe_data (frame), sizeof (uint64_t));
    char *id;
    id = router_route (compute->router, key);

    //check that self has been initialized
    assert (compute->router->self != NULL);

    //send to self if necessary
    if (strcmp (compute->router->self->key, id) == 0) {
	if (wb) {
	    zmsg_send (&msg, compute->self_wb);
	}
	else {
	    zmsg_send (&msg, compute->self_nb);
	}

    }
    else {
	//write address
	zmsg_wrap (msg, zframe_new (id, strlen (id)));
	if (wb) {
	    zmsg_send (&msg, compute->socket_wb);
	}
	else {
	    zmsg_send (&msg, compute->socket_nb);
	}

    }



}

//returns the new interval or -1 on error
int
worker_new_interval (worker_t * worker, localdb_t * localdb)
{

//getting the interval and trying to update it until it works

    while (1) {
	int result;
	struct Stat stat;

	char path[1000];
	char octopus[1000];
	int buffer;
	int buffer_len = sizeof (int);

	oconfig_octopus (worker->config, octopus);

	sprintf (path, "/%s/global_properties/interval/last_interval",
		 octopus);
	result =
	    zoo_get (worker->zh, path, 0, (char *) &buffer, &buffer_len,
		     &stat);
	assert (result == ZOK);

	assert (buffer_len == sizeof (int));

	buffer++;

	result = zoo_set (worker->zh, path, (const char *) &buffer,
			  buffer_len, stat.version);

	if (ZBADVERSION == result) {
	}
	else {
	    assert (result == ZOK);


	    localdb_set_interval (localdb, buffer);
	    return buffer;
	}

    }

}

int
compute (zloop_t * loop, zmq_pollitem_t * item, void *arg)
{


    return 0;
}

//th structure of a msg is this
//unique id
//node address
// the rest

//if node adress equals yourself then it is a confirmation
// the first 2 identify the kind of message

void
worker_balance (balance_t * balance)
{

    zmsg_t *msg = zmsg_recv (balance->self_bl);
    if (!msg) {
	exit (1);
    }

    zmsg_t *responce;

    zframe_t *address = zmsg_unwrap (msg);


//clean this TODO
    zframe_t *type_fr = zmsg_pop (msg);
    zframe_t *id_frame = zmsg_first (msg);
    zframe_t *key_frame = zmsg_next (msg);
    zframe_t *frame = key_frame;

    if (strcmp ((const char *) zframe_data (frame), balance->self_key) == 0) {
// this is confirmation

	//identify the on_give object
	on_give_t *iter = zlist_first (balance->on_gives);
	while (iter) {
	    if ((memcmp
		 (zframe_data (id_frame), &(iter->un_id),
		  sizeof (int)) == 0)) {


		if (memcmp (INTERVAL_RECEIVED, zframe_data (type_fr), 1) == 0) {
		    // send the chunks


		    responce = zmsg_new ();
		    frame = zframe_new (NEW_CHUNK, 1);
		    zmsg_add (responce, frame);
		    frame = zframe_dup (id_frame);
		    zmsg_add (responce, frame);
		    frame = zframe_dup (key_frame);
		    zmsg_add (responce, frame);




		    interval_t *interval;
		    interval_init (&interval, &(iter->event->start),
				   &(iter->event->end));

		    khint_t hiter;
		    uint64_t key;
		    vertex_t *vertex;
		    uint64_t counter = 1;
		    uint64_t vertices = 0;
		    zmsg_t *responce_dup = zmsg_dup (responce);
		    frame = zframe_new (&counter, sizeof (uint64_t));
		    zmsg_add (responce_dup, frame);

		    for (hiter = kh_begin (balance->hash);
			 hiter != kh_end (balance->hash); ++hiter) {
			if (!kh_exist (balance->hash, hiter))
			    continue;

			key = kh_key (balance->hash, hiter);
			if (interval_belongs (interval, key)) {
			    vertices++;
			    vertex_init (&vertex);
			    memcpy (vertex, &(kh_val (balance->hash, hiter)),
				    sizeof (vertex_t));
			    //add it to the list of sent vertices     
			    zlist_append (iter->unc_vertices, vertex);
			    //add it
			    zmsg_add (responce_dup,
				      zframe_new (vertex, sizeof (vertex_t)));
			    if (counter < 1 + vertices / COUNTER_SIZE) {
				zmsg_wrap (responce_dup, address);
				zmsg_send (&responce_dup, balance->router_bl);
				counter++;
				responce_dup = zmsg_dup (responce);
				frame =
				    zframe_new (&counter, sizeof (uint64_t));
				zmsg_add (responce_dup, frame);


			    }




			}
		    }


		    zmsg_destroy (&responce);


		    free (interval);
		}
		if (memcmp (CONFIRM_CHUNK, zframe_data (type_fr), 1) == 0) {
		    //delete the vertices that have been verified  
		    frame = zmsg_next (msg);
		    uint64_t counter;
		    memcpy (&counter, zframe_data (frame), sizeof (uint64_t));
		    if (counter != 0) {
			uint64_t diff;
			diff = counter - iter->rec_counter;
			iter->rec_counter = counter;
			int i;
			for (i = 0; i < diff * COUNTER_SIZE; i++) {
			    free (zlist_pop (iter->unc_vertices));
			}

		    }
		    else {

			while (zlist_size (iter->unc_vertices)) {
			    free (zlist_pop (iter->unc_vertices));
			}
			//remove the event form the event list
			zlist_remove (balance->events, iter->event);
			on_give_destroy (iter);

		    }


		}

		if (memcmp (MISSED_CHUNKES, zframe_data (type_fr), 1) == 0) {
		    responce = zmsg_new ();
		    frame = zframe_new (NEW_CHUNK, 1);
		    zmsg_add (responce, frame);
		    frame = zframe_dup (id_frame);
		    zmsg_add (responce, frame);
		    frame = zframe_dup (key_frame);
		    zmsg_add (responce, frame);





		    //send the missed pieces
		    uint64_t diff = 0;
		    while ((frame = zmsg_next (msg))) {
			uint64_t counter;
			memcpy (&counter, zframe_data (frame),
				sizeof (uint64_t));

			zmsg_t *responce_dup = zmsg_dup (responce);
			zmsg_add (responce_dup, frame);

			//we expect the counter to be of increasing order
			diff = counter - diff - iter->rec_counter;
			vertex_t *vertex = zlist_first (iter->unc_vertices);
			if (diff > 1) {
			    int i;
			    for (i = 1; i < (diff - 1) * COUNTER_SIZE; i++) {
				zlist_next (iter->unc_vertices);
			    }
			    vertex = zlist_next (iter->unc_vertices);
			}
			zmsg_add (responce_dup,
				  zframe_new (vertex, sizeof (vertex_t)));
			int i;
			for (i = 1; i < COUNTER_SIZE; i++) {
			    zmsg_add (responce_dup,
				      zframe_new (zlist_next
						  (iter->unc_vertices),
						  sizeof (vertex_t)));
			}


			zmsg_wrap (responce_dup, address);
			zmsg_send (&responce_dup, balance->router_bl);















		    }
		    zmsg_destroy (&responce);
		}


		break;
	    }
	    iter = zlist_next (balance->on_gives);
	}
	zmsg_destroy (&msg);
    }
    else {
	if (memcmp (NEW_CHUNK, zframe_data (type_fr), 1) == 0) {

//a previously initiated session(on_receive)

	    on_receive_t *iter = zlist_first (balance->on_receives);
	    while (iter) {
		if ((strcmp
		     ((const char *) zframe_data (frame),
		      iter->action->key) == 0)
		    &&
		    (memcmp
		     (zframe_data (id_frame), &(iter->un_id),
		      sizeof (int)) == 0)) {
		    frame = zmsg_next (msg);

		    uint64_t count;
		    memcpy (&count, zframe_data (frame), sizeof (uint64_t));
		    if (count == 0) {
//EOTransm. signal
//if there are missing chunks request them,otherwise free the event 

			if (0 == zlist_size (iter->m_counters)) {

			    responce = zmsg_new ();
			    frame = zframe_new (CONFIRM_CHUNK, 1);
			    zmsg_add (responce, frame);
			    frame = zframe_dup (id_frame);
			    zmsg_add (responce, frame);
			    frame = zframe_dup (key_frame);
			    zmsg_add (responce, frame);
			    uint64_t zero = 0;
			    frame = zframe_new (&zero, sizeof (uint64_t));
			    zmsg_add (responce, frame);
			    zmsg_wrap (responce, address);
			    zmsg_send (&responce, balance->router_bl);

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
				    events_possible (balance->events,
						     balance->intervals))) {
//perform this event

//update the intervals

				interval_t *interval;
				interval_init (&interval,
					       &(event->start),
					       &(event->end));
				intervals_remove (balance->intervals,
						  interval);

//update un_id;
				if (balance->un_id > 1000000000) {
				    balance->un_id = 1;
				}
				else {
				    balance->un_id++;
				}


//create on_give object
				on_give_t *on_give;
				on_give_init (&on_give, event,
					      balance->un_id);

//update balance object
				balance_update (balance, on_give);


//put on_give event into the list
				zlist_append (balance->on_gives, on_give);
			    }





			    //destroy on_receive
			    on_receive_destroy (iter);




			}
			else {


			    responce = zmsg_new ();
			    frame = zframe_new (MISSED_CHUNKES, 1);
			    zmsg_add (responce, frame);
			    frame = zframe_dup (id_frame);
			    zmsg_add (responce, frame);
			    frame = zframe_dup (key_frame);
			    zmsg_add (responce, frame);

			    uint64_t *citer = zlist_first (iter->m_counters);
			    while (citer) {
				frame = zframe_new (citer, sizeof (uint64_t));
				zmsg_add (responce, frame);
				citer = zlist_next (iter->m_counters);

			    }

			    zmsg_wrap (responce, address);
			    zmsg_send (&responce, balance->router_bl);





			}




//this is necessary  ur put an else {}
			break;
		    }

		    if (count - iter->counter > 1) {
//I have missed the previous chunck

			uint64_t *counter = malloc (sizeof (uint64_t));
			memcpy (counter, zframe_data (frame),
				sizeof (uint64_t));
			zlist_append (iter->m_counters, counter);
		    }

		    int resent = 0;	//all ready got it

		    if (count - iter->counter < 1) {
//I received a lost chunk
			resent = 1;

//remove the counter TODO not very efficient
			uint64_t *citer = zlist_first (iter->m_counters);
			while (citer) {
			    if (count == *citer) {
				zlist_remove (iter->m_counters, citer);
				free (citer);
				resent = 0;
				break;
			    }
			    citer = zlist_next (iter->m_counters);
			}
		    }
		    if (count - iter->counter == 1) {
			iter->counter++;


//send confirmation
			responce = zmsg_new ();
			frame = zframe_new (CONFIRM_CHUNK, 1);
			zmsg_add (responce, frame);
			frame = zframe_dup (id_frame);
			zmsg_add (responce, frame);
			frame = zframe_dup (key_frame);
			zmsg_add (responce, frame);
			frame =
			    zframe_new (&(iter->counter), sizeof (uint64_t));
			zmsg_add (responce, frame);
			zmsg_wrap (responce, address);
			zmsg_send (&responce, balance->router_bl);

		    }

		    if (resent == 0) {
//save the new data
			frame = zmsg_next (msg);
			while (frame) {
			    int ret;
			    uint64_t key;
			    memcpy (&key, zframe_data (frame),
				    sizeof (uint64_t));
			    khiter_t k =
				kh_put (vertices, balance->hash, key, &ret);
			    assert (ret != 0);
			    frame = zmsg_next (msg);
			    memcpy (&kh_value (balance->hash, k),
				    zframe_data (frame), sizeof (vertex_t));
			    frame = zmsg_next (msg);
			}


		    }

		    zmsg_destroy (&msg);

		    break;
		}
		iter = zlist_next (balance->on_receives);
	    }
	    //in case no on_receive events exists, drop things

	    zmsg_destroy (&msg);
	}
	else {
//TODO check if I had already received that NEW_INTERVAL
//after all we use lazy pirate

	    assert (memcmp (NEW_INTERVAL, zframe_data (type_fr), 1) == 0);
//this is an interval initiation msg

//update intervals, now the node has taken responsibility of the interval
	    interval_t *interval;
	    interval_minit (&interval, msg);
	    intervals_add (balance->intervals, interval);




	    on_receive_t *on_receive;
//on_receive destroys the msg
	    on_receive_init (&on_receive, msg);
	    zlist_append (balance->on_receives, on_receive);


//send confirmation
	    responce = zmsg_new ();
	    frame = zframe_new (INTERVAL_RECEIVED, 1);
	    zmsg_add (responce, frame);
	    frame = zframe_dup (id_frame);
	    zmsg_add (responce, frame);
	    frame = zframe_dup (key_frame);
	    zmsg_add (responce, frame);
	    zmsg_wrap (responce, address);
	    zmsg_send (&responce, balance->router_bl);

	}


    }
    zframe_destroy (&type_fr);

}


void
worker_balance_update (balance_t * balance)
{


    on_give_t *iter = zlist_first (balance->on_gives);

    while (iter) {

	zmsg_t *msg = zmsg_new ();
	zframe_t *frame;
	zframe_t *address;

	int64_t time = zclock_time ();

	if (iter->state != 1) {

	    if (time - iter->last_time > ONGOING_TIMEOUT) {
		if (iter->state == 0) {

		    fprintf (stderr,
			     "sending NEW_INTERVAL msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
			     iter->event->key, iter->event->start.prefix,
			     iter->event->start.suffix,
			     iter->event->end.prefix,
			     iter->event->end.suffix);


		    frame = zframe_new (NEW_INTERVAL, 1);
		    zmsg_add (msg, frame);
		    frame = zframe_new (&(iter->un_id), sizeof (int));
		    zmsg_add (msg, frame);
		    frame = zframe_new (balance->self_key, strlen (balance->self_key) + 1);	//this also contains the null char
		    zmsg_add (msg, frame);	//because it is not used as address
		    frame =
			zframe_new (&(iter->event->start),
				    sizeof (struct _hkey_t));
		    zmsg_add (msg, frame);
		    frame =
			zframe_new (&(iter->event->end),
				    sizeof (struct _hkey_t));
		    zmsg_add (msg, frame);


		    address =
			zframe_new (iter->event->key,
				    strlen (iter->event->key));

		    zmsg_wrap (msg, address);

		    zmsg_send (&msg, balance->router_bl);
		}

		if (iter->state == 2) {
		    fprintf (stderr,
			     "sending EOT msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
			     iter->event->key, iter->event->start.prefix,
			     iter->event->start.suffix,
			     iter->event->end.prefix,
			     iter->event->end.suffix);


		    frame = zframe_new (NEW_CHUNK, 1);
		    zmsg_add (msg, frame);
		    frame = zframe_new (&(iter->un_id), sizeof (int));
		    zmsg_add (msg, frame);
		    frame = zframe_new (balance->self_key, strlen (balance->self_key) + 1);	//this also contains the null char
		    zmsg_add (msg, frame);	//because it is not used as address


		    uint64_t counter = 0;

		    frame = zframe_new (&counter, sizeof (uint64_t));
		    zmsg_add (msg, frame);



		    address =
			zframe_new (&(iter->event->key),
				    strlen (iter->event->key));

		    zmsg_wrap (msg, address);

		    zmsg_send (&msg, balance->router_bl);

		}
	    }
	    else {
//update the timeout
		if (balance->timeout >
		    ONGOING_TIMEOUT - time + iter->last_time) {
		    balance->timeout =
			ONGOING_TIMEOUT - time + iter->last_time;
		}
	    }

	}


	iter = zlist_next (balance->on_gives);
    }

}



void
update_n_pieces (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    int n_pieces;

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);
    node_t *prev_node;
    node = node_dup (prev_node = nodes_search (update->router->nodes, key));
    assert (prev_node != NULL);
    node->n_pieces = n_pieces;

//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 0);

//update router object
//this should always happen after the prev step

    router_delete (update->router, prev_node);
    router_add (update->router, node);

    event_t *event;
    while ((event = zlist_pop (events))) {
//check if there is an action that this event describes
	if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
	    if (event_possible (event, update->balance->intervals)) {
//perform this event

//update the intervals

		interval_t *interval;
		interval_init (&interval, &(event->start), &(event->end));
		intervals_remove (update->balance->intervals, interval);

//update un_id
		if (update->balance->un_id > 1000000000) {
		    update->balance->un_id = 1;
		}
		else {
		    update->balance->un_id++;
		}


//create on_give object
		on_give_t *on_give;
		on_give_init (&on_give, event, update->balance->un_id);

//update balance object
		balance_update (update->balance, on_give);

//put event into the events list
//not necessary
		zlist_append (update->balance->events, event);

//put on_give event into the list
		zlist_append (update->balance->on_gives, on_give);

	    }
	    else {
//add the event to the events
		zlist_append (update->balance->events, event);

	    }



	}
	else {
//do nothing, the event has already happened
	    free (event);
	}
    }
    zlist_destroy (&events);

    intervals_print (update->balance->intervals);

    fprintf (stderr, "\nWorker with id: %s has updated its n_pieces to %d.",
	     update->balance->self_key, n_pieces);
}



void
update_st_piece (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    unsigned long st_piece;

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);
    node_t *prev_node;
    node = node_dup (prev_node = nodes_search (update->router->nodes, key));
    assert (prev_node != NULL);
    node->st_piece = st_piece;

//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 0);

//update router object
//this should always happen after the prev step

    router_delete (update->router, prev_node);
    router_add (update->router, node);

    event_t *event;
    while ((event = zlist_pop (events))) {
//check if there is an action that this event describes
	if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
	    if (event_possible (event, update->balance->intervals)) {
// perform this event

//update the intervals

		interval_t *interval;
		interval_init (&interval, &(event->start), &(event->end));
		intervals_remove (update->balance->intervals, interval);

//update un_id;
		if (update->balance->un_id > 1000000000) {
		    update->balance->un_id = 1;
		}
		else {
		    update->balance->un_id++;
		}


//create on_give object
		on_give_t *on_give;
		on_give_init (&on_give, event, update->balance->un_id);

//update balance object
		balance_update (update->balance, on_give);

//put event into the events list
//not necessary
		zlist_append (update->balance->events, event);

//put on_give event into the list
		zlist_append (update->balance->on_gives, on_give);

	    }
	    else {
//add the event to the events
		zlist_append (update->balance->events, event);

	    }



	}
	else {
//do nothing, the event has already happened
	    free (event);

	}
    }
    zlist_destroy (&events);

    intervals_print (update->balance->intervals);

    fprintf (stderr,
	     "\nWorker with id: %s has incremented the st_piece of the worker with id:%s to: %lu.",
	     update->balance->self_key, node->key, st_piece);

}


void
remove_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node = nodes_search (update->router->nodes, key);

    assert (node != NULL);

//remove all previous events by this node. they will never happen
//the node is dead

    events_remove (update->balance->events, node);

    int rc;
    rc = zsocket_disconnect (update->compute->socket_nb, "%s",
			     node->bind_point_nb);
    assert (rc == 0);
    rc = zsocket_disconnect (update->compute->socket_wb, "%s",
			     node->bind_point_wb);
    assert (rc == 0);
    rc = zsocket_disconnect (update->balance->router_bl, "%s",
			     node->bind_point_bl);
    assert (rc == 0);



//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 1);

//update router object
//this should always happen after the prev step
    router_delete (update->router, node);

    fprintf (stderr, "\nremove_node:worker_update:size of event list: %lu",
	     zlist_size (events));
    event_t *event = zlist_first (events);
    int iter = 0;
    while (event) {
	iter++;
	fprintf (stderr,
		 "\nevent %d \n start: %lu %lu \n end: %lu %lu \n key: %s \n give: %d",
		 iter, event->start.prefix, event->start.suffix,
		 event->end.prefix, event->end.suffix, event->key,
		 event->give);
	event = zlist_next (events);
    }



    while ((event = zlist_pop (events))) {

	assert (event->give == 0);
//the only possible thing that could happen would be to add 
//the new intervals to this node

	interval_t *interval;
	interval_init (&interval, &(event->start), &(event->end));

	intervals_add (update->balance->intervals, interval);
	free (event);




    }


    zlist_destroy (&events);


    intervals_print (update->balance->intervals);

    fprintf (stderr, "\nWorker with id: %s has removed the node with id %s.",
	     update->balance->self_key, key);

}









void
add_node (update_t * update, zmsg_t * msg)
{
    int start;
    node_t *node;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point_nb[50];
    char bind_point_wb[50];
    char bind_point_bl[50];

    zframe_t *frame = zmsg_first (msg);
    memcpy (&start, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_nb, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_wb, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_bl, zframe_data (frame), zframe_size (frame));


    zmsg_destroy (&msg);

//connect to the node

    int rc;
    rc = zsocket_connect (update->compute->socket_nb, "%s", bind_point_nb);
    assert (rc == 0);
    rc = zsocket_connect (update->compute->socket_wb, "%s", bind_point_wb);
    assert (rc == 0);
    rc = zsocket_connect (update->balance->router_bl, "%s", bind_point_bl);
    assert (rc == 0);




    fprintf (stderr,
	     "\nworker_add_node\nstart:%d\nkey:%s\nn_pieces:%d\nst_piece:%lu",
	     start, key, n_pieces, st_piece);

    node_init (&node, key, n_pieces, st_piece, bind_point_nb, bind_point_wb,
	       bind_point_bl);

    zlist_t *events;
    if (!start) {
//obtain the new events
	events = router_events (update->router, node, 0);
    }

//update router object
//this should always happen after the prev step
    assert (1 == router_add (update->router, node));

    if (!start) {
	fprintf (stderr, "\nworker_update:size of event list: %lu",
		 zlist_size (events));
	event_t *event = zlist_first (events);
	int iter = 0;
	while (event) {
	    iter++;
	    fprintf (stderr,
		     "\nevent %d \n start: %lu %lu \n end: %lu %lu \n key: %s \n give: %d",
		     iter, event->start.prefix, event->start.suffix,
		     event->end.prefix, event->end.suffix, event->key,
		     event->give);
	    event = zlist_next (events);
	}
	event = zlist_pop (events);
	if (event && (strcmp (event->key, "\0") == 0)) {

	    interval_t *interval;
	    interval_init (&interval, &(event->start), &(event->end));
	    intervals_add (update->balance->intervals, interval);
	    free (event);
	    event = zlist_pop (events);
	    assert (event == NULL);
	}

	while (event) {

//check if there is an action that this event describes
	    if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
		if (event_possible (event, update->balance->intervals)) {
// perform this event

//update the intervals

		    interval_t *interval;
		    interval_init (&interval, &(event->start), &(event->end));
		    intervals_remove (update->balance->intervals, interval);

//update un_id;
		    if (update->balance->un_id > 1000000000) {
			update->balance->un_id = 1;
		    }
		    else {
			update->balance->un_id++;
		    }


//create on_give object
		    on_give_t *on_give;
		    on_give_init (&on_give, event, update->balance->un_id);

//update balance object
		    balance_update (update->balance, on_give);

//put event into the events list
//not necessary
		    zlist_append (update->balance->events, event);

//put on_give event into the list
		    zlist_append (update->balance->on_gives, on_give);


		}
		else {
//add the event to the events
		    zlist_append (update->balance->events, event);

		}



	    }
	    else {
//do nothing, the event has already happened
		free (event);
	    }
	    event = zlist_pop (events);
	}
	zlist_destroy (&events);

    }
    intervals_print (update->balance->intervals);

    fprintf (stderr, "\nWorker with id: %s has added the node with id %s.",
	     update->balance->self_key, key);

}


void
add_self (update_t * update, zmsg_t * msg)
{
    node_t *self;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point_nb[50];
    char bind_point_wb[50];
    char bind_point_bl[50];



    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_nb, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_wb, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_bl, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    fprintf (stderr, "\nworker_add_self\nkey:%s\nn_pieces:%d\nst_piece:%lu",
	     key, n_pieces, st_piece);

    node_init (&self, key, n_pieces, st_piece, bind_point_nb, bind_point_wb,
	       bind_point_bl);

//update router
    update->router->self = self;

//the rest of the router update will happen when the node goes online

//bind sockets
    int rc;
    rc = zsocket_bind (update->compute->self_nb, "%s", bind_point_nb);
    assert (rc != -1);
    rc = zsocket_bind (update->compute->self_wb, "%s", bind_point_wb);
    assert (rc != -1);
    rc = zsocket_connect (update->compute->socket_nb, "%s", bind_point_nb);
    assert (rc == 0);
    rc = zsocket_connect (update->compute->socket_wb, "%s", bind_point_wb);
    assert (rc == 0);
    rc = zsocket_bind (update->balance->self_bl, "%s", bind_point_bl);
    assert (rc != -1);
    rc = zsocket_connect (update->balance->router_bl, "%s", bind_point_bl);
    assert (rc == 0);

    fprintf (stderr, "\nWorker with id: %s has received its configuration",
	     update->balance->self_key);


}

//this sets the node up
//the event of that action hasnt though come from zookeeper
//it doesnt exist in the "view" of this node
void
go_online (worker_t * worker)
{

    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (worker->config, octopus);
    oconfig_comp_name (worker->config, comp_name);

    sprintf (path, "/%s/computers/%s/worker_nodes/%s/online", octopus,
	     comp_name, worker->res_name);

    int result = zoo_create (worker->zh, path, NULL,
			     -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL,
			     0);
    assert (result == ZOK);

    fprintf (stderr, "\nWorker with id: %s has gone online", worker->id);


}




int
worker_update (update_t * update, void *sub)
{

//check if it is a new update or an old one
    zmsg_t *msg = zmsg_recv (sub);
    if (!msg) {
	exit (1);
    }

    fprintf (stderr, "\nworker_update:I have received a sub msg");
    zframe_t *sub_frame = zmsg_pop (msg);
    zframe_destroy (&sub_frame);
    zframe_t *id = zmsg_pop (msg);
    if (memcmp (zframe_data (id), &(update->id), sizeof (unsigned int)) == 0) {
//lazy pirate reconfirm update
	zframe_send (&id, update->dealer, 0);
	zmsg_destroy (&msg);
	fprintf (stderr,
		 "\nworker_update:It was a previous update, resending confirmation");

    }
    else {
	zframe_t *frame = zmsg_pop (msg);
	if (memcmp (zframe_data (frame), "add_self", zframe_size (frame)) ==
	    0) {
	    add_self (update, msg);
	}
	else {
	    if (memcmp
		(zframe_data (frame), "remove_node",
		 zframe_size (frame)) == 0) {
		remove_node (update, msg);
	    }
	    else {
		if (memcmp
		    (zframe_data (frame), "add_node",
		     zframe_size (frame)) == 0) {
		    add_node (update, msg);
		}
		else {
		    if (memcmp
			(zframe_data (frame), "st_piece",
			 zframe_size (frame)) == 0) {
			update_st_piece (update, msg);
		    }
		    else {
			if (memcmp
			    (zframe_data (frame), "n_pieces",
			     zframe_size (frame)) == 0) {
			    update_n_pieces (update, msg);
			}
			else {
			    if (memcmp
				(zframe_data (frame), "go_online",
				 zframe_size (frame)) == 0) {
				go_online (update->compute->worker);
			    }




			}
		    }
		}
	    }
	}


	zframe_destroy (&frame);


	zframe_send (&id, update->dealer, 0);
	fprintf (stderr,
		 "\nworker_update:I have sent confirmation to sub msg");

    }

    return 0;
}


void
worker_sleep (sleep_t * sleep, compute_t * compute)
{
    zmsg_t *msg;
    unsigned short wb;

    while ((msg = sleep_awake (sleep, &wb))) {
	worker_send (msg, wb, compute);

    }


}



void *
worker_fn (void *arg)
{
    zctx_t *ctx = zctx_new ();

    worker_t *worker = (worker_t *) arg;

    int rc;
//update infrastructure
    void *sub = zsocket_new (ctx, ZMQ_SUB);
    void *dealer = zsocket_new (ctx, ZMQ_DEALER);


    zmq_setsockopt (dealer, ZMQ_IDENTITY, worker->id, strlen (worker->id));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, worker->id, strlen (worker->id));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "all", strlen ("all") + 1);


    rc = zsocket_connect (sub, "tcp://127.0.0.1:49152");
    assert (rc == 0);

    rc = zsocket_connect (dealer, "tcp://127.0.0.1:49153");
    assert (rc == 0);




//worker infrastruct

    void *socket_wb = zsocket_new (ctx, ZMQ_ROUTER);
    void *socket_nb = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_wb = zsocket_new (ctx, ZMQ_DEALER);
    void *self_nb = zsocket_new (ctx, ZMQ_DEALER);

    char *identity = malloc (1000);

    sprintf (identity, "%swb", worker->id);
    zmq_setsockopt (self_wb, ZMQ_IDENTITY, identity, strlen (identity));
    sprintf (identity, "%snb", worker->id);
    zmq_setsockopt (self_nb, ZMQ_IDENTITY, identity, strlen (identity));

//balance infrastructure

    void *router_bl = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_bl = zsocket_new (ctx, ZMQ_DEALER);
    zmq_setsockopt (self_bl, ZMQ_IDENTITY, worker->id, strlen (worker->id));

//cleaning
    free (identity);

//hash of vertices
    khash_t (vertices) * hash = kh_init (vertices);

//sleep object
    sleep_t *sleep;
    sleep_init (&sleep);

//router object
//used to find where each msg goes
    router_t *router;

    router_init (&router, 0);


//balance object
    balance_t *balance;

    balance_init (&balance, hash, router_bl, self_bl, worker->id);

//localdb object
//used to save the counter used to create new vertices
    localdb_t *localdb;
    localdb_init (&localdb, worker->id);

//compute object
    compute_t *compute;

    compute_init (&compute, hash, router, balance->events, balance->intervals,
		  socket_nb, self_nb, socket_wb, self_wb, localdb, worker);

//update object
//used to update things, like the router object
    update_t *update;


    update_init (&update, dealer, router, balance, compute);

    zmq_pollitem_t pollitems[4] =
	{ {sub, 0, ZMQ_POLLIN}, {self_bl, 0, ZMQ_POLLIN}, {self_wb, 0,
							   ZMQ_POLLIN},
    {self_nb, 0, ZMQ_POLLIN}
    };
    fprintf (stderr, "\nworker with id:%s ready.", worker->id);
//main loop
    while (1) {
//finding the minimum timeout
	int who = 0;
	int64_t timeout = balance->timeout;
	if ((sleep->timeout > 0 && sleep->timeout < balance->timeout)
	    || (balance->timeout < 0)) {
	    timeout = sleep->timeout;
	    who = 1;
	}
	rc = zmq_poll (pollitems, 4, timeout);
	assert (rc != -1);

//sends all msgs that their delay has expired
	if (who) {
	    worker_sleep (sleep, compute);
	}
	else {
	    worker_balance_update (balance);
	}

	if (pollitems[0].revents & ZMQ_POLLIN) {
	    worker_update (update, sub);
	}
	if (pollitems[1].revents & ZMQ_POLLIN) {
	    worker_balance (balance);
	}
	if (pollitems[2].revents & ZMQ_POLLIN) {

	}
	if (pollitems[3].revents & ZMQ_POLLIN) {

	}




    }





}

void
compute_init (compute_t ** compute, khash_t (vertices) * hash,
	      router_t * router, zlist_t * events, intervals_t * intervals,
	      void *socket_nb, void *self_nb, void *socket_wb, void *self_wb,
	      localdb_t * localdb, worker_t * worker)
{

    *compute = malloc (sizeof (compute_t));

    (*compute)->router = router;
    (*compute)->events = events;
    (*compute)->intervals = intervals;
    (*compute)->socket_nb = socket_nb;
    (*compute)->self_nb = self_nb;
    (*compute)->socket_wb = socket_wb;
    (*compute)->self_wb = self_wb;
    (*compute)->worker = worker;
    (*compute)->hash = hash;
    (*compute)->localdb = localdb;
    (*compute)->interval = localdb_get_interval (localdb);
    (*compute)->counter = localdb_get_counter (localdb);
//in case we just started
    if ((*compute)->interval == -1) {
	(*compute)->interval = worker_new_interval (worker, localdb);
//init counter
	localdb_incr_counter (localdb, 1);
	(*compute)->counter = 1;
    }

//getting the interval size

    int result;
    struct Stat stat;

    char path[1000];
    char octopus[1000];
    unsigned long buffer;
    int buffer_len = sizeof (unsigned long);

    oconfig_octopus (worker->config, octopus);

    sprintf (path, "/%s/global_properties/interval/interval_size", octopus);
    result = zoo_get (worker->zh, path, 0, (char *) buffer,
		      &buffer_len, &stat);
    assert (result == ZOK);

    assert (buffer_len == sizeof (unsigned long));

    (*compute)->interval_size = buffer;



}



void
worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config,
	     char *comp_name, char *res_name)
{

    *worker = malloc (sizeof (worker_t));
    (*worker)->zh = zh;
    (*worker)->res_name = malloc (strlen (res_name) + 1);
    strcpy ((*worker)->res_name, res_name);
    (*worker)->comp_name = malloc (strlen (comp_name) + 1);
    strcpy ((*worker)->comp_name, comp_name);
    (*worker)->id = malloc (strlen (comp_name) + strlen (res_name) + 1);
    sprintf ((*worker)->id, "%s%s", comp_name, res_name);
    (*worker)->config = config;
}

void
workers_init (workers_t ** workers, ozookeeper_t * ozookeeper)
{

    int result;
    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);

    sprintf (path, "/%s/computers/%s/worker_nodes", octopus, comp_name);
    struct String_vector worker_children;
    result = zoo_get_children (ozookeeper->zh, path, 0, &worker_children);


    if (ZOK == result) {

//mallocing
	*workers = malloc (sizeof (workers_t));
	(*workers)->size = worker_children.count;
	(*workers)->id = malloc (sizeof (char *) * (worker_children.count));
	(*workers)->pthread =
	    malloc (sizeof (pthread_t) * worker_children.count);

//create the threads

	int iter;
	if (worker_children.count < 1000) {
	    worker_t *worker;
	    for (iter = 0; iter < worker_children.count; iter++) {
		(*workers)->id[iter] =
		    malloc (strlen (worker_children.data[iter]) + 1 +
			    strlen (comp_name));

		sprintf ((*workers)->id[iter], "%s%s", comp_name,
			 worker_children.data[iter]);

		worker_init (&worker, ozookeeper->zh, ozookeeper->config,
			     comp_name, worker_children.data[iter]
		    );

		pthread_create (&((*workers)->pthread[iter]), NULL, worker_fn,
				worker);
	    }
	}
	else {
	    fprintf (stderr, "\n More workers than allowed.. error exiting");
	    exit (1);
	}
	if (ZOK != result && ZNONODE != result) {
	    fprintf (stderr, "\n Couldnt get the children.. error exiting");
	    exit (1);
	}


	deallocate_String_vector (&worker_children);
    }




}
