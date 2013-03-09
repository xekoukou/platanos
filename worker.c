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

#include"worker.h"

//TODO this is arbitrary
#define ONGOING_TIMEOUT 4000
#define COUNTER_SIZE 1000       /* 1000 vertices per chunk */

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

        sprintf (path, "/%s/global_properties/interval/last_interval", octopus);
        result =
            zoo_get (worker->zh, path, 0, (char *) &buffer, &buffer_len, &stat);
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

    balance_new_msg (balance, msg);

}


void
worker_balance_lazy_pirate (balance_t * balance)
{

    balance_lazy_pirate (balance);



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
    int circle;
    events = router_events (update->router, node, 0, &circle);

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
                balance_update_give_timer (update->balance, on_give);

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
    int circle;
    events = router_events (update->router, node, 0, &circle);

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
                balance_update_give_timer (update->balance, on_give);

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

    on_gives_remove (update->balance->on_gives, update->balance->events, node);

    on_receives_destroy (update->balance->on_receives, update->balance, node);

//remove all previous events by this node. they will never happen
//the node is dead

    events_remove (update->balance->events, node);

    //  int rc;
    //  rc = zsocket_disconnect (update->compute->socket_nb, "%s",
    //                         node->bind_point_nb);
    //  assert (rc == 0);
    //  rc = zsocket_disconnect (update->compute->socket_wb, "%s",
    //                         node->bind_point_wb);
    //  assert (rc == 0);
    //  rc = zsocket_disconnect (update->balance->router_bl, "%s",
    //                         node->bind_point_bl);
    //  assert (rc == 0);



//obtain the new events
    zlist_t *events;
    int circle;
    events = router_events (update->router, node, 1, &circle);

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
                 event->end.prefix, event->end.suffix, event->key, event->give);
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
    int circle;
    if (!start) {
//obtain the new events
        events = router_events (update->router, node, 0, &circle);
        //router_events emits a circle when the event is the full circle, and emits null events
        //thus the action taken when circle is 0 is to evaluate the events themselves,
        //those will change the first circle into 0 for example
        if (circle) {
            update->balance->intervals->circle = circle;
        }
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
                    balance_update_give_timer (update->balance, on_give);

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

void
db_add_node (update_t * update, zmsg_t * msg)
{
    int start;
    node_t *node;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point_db[50];

    zframe_t *frame = zmsg_first (msg);
    memcpy (&start, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_db, zframe_data (frame), zframe_size (frame));


    zmsg_destroy (&msg);

//connect to the node

    int rc;
    rc = zsocket_connect (update->compute->socket_nb, "%s", bind_point_db);
    assert (rc == 0);
    rc = zsocket_connect (update->compute->socket_wb, "%s", bind_point_db);
    assert (rc == 0);




    fprintf (stderr,
             "\nworker_db_add_node\nstart:%d\nkey:%s\nn_pieces:%d\nst_piece:%lu",
             start, key, n_pieces, st_piece);

    db_node_init (&node, key, n_pieces, st_piece, bind_point_db);

//update router object
//this should always happen after the prev step
    assert (1 == router_add (update->db_router, node));
}

void
db_update_st_piece (update_t * update, zmsg_t * msg)
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
    node = node_dup (prev_node = nodes_search (update->db_router->nodes, key));
    assert (prev_node != NULL);
    node->st_piece = st_piece;

//update router object
//this should always happen after the prev step

    router_delete (update->db_router, prev_node);
    router_add (update->db_router, node);

}

void
db_update_n_pieces (update_t * update, zmsg_t * msg)
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
    node = node_dup (prev_node = nodes_search (update->db_router->nodes, key));
    assert (prev_node != NULL);
    node->n_pieces = n_pieces;

//update router object
//this should always happen after the prev step

    router_delete (update->db_router, prev_node);
    router_add (update->db_router, node);
}


void
db_remove_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node = nodes_search (update->db_router->nodes, key);

    assert (node != NULL);

    node_set_alive (node, 0);


}


void
db_delete_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node = nodes_search (update->db_router->nodes, key);

    assert (node != NULL);

//update router object
//this should always happen after the prev step
    router_delete (update->db_router, node);


}


//this only updates the router object of the database
//thus it rejects the add_self data
//and the get_online data
void
worker_update_db (update_t * update, zmsg_t * msg)
{

    zframe_t *id = zmsg_pop (msg);
    if (memcmp (zframe_data (id), &(update->id), sizeof (unsigned int)) == 0) {
//lazy pirate reconfirm update
        zframe_send (&id, update->dealer, 0);
        zframe_destroy (&id);
        zmsg_destroy (&msg);
        fprintf (stderr,
                 "\nworker_update_db:It was a previous update, resending confirmation");

    }
    else {
        zframe_t *frame = zmsg_pop (msg);
        if (memcmp (zframe_data (frame), "add_self", zframe_size (frame)) == 0) {
            zmsg_destroy (&msg);
        }
        else {
            if (memcmp
                (zframe_data (frame), "delete_node",
                 zframe_size (frame)) == 0) {
                db_delete_node (update, msg);
            }
            else {

                if (memcmp
                    (zframe_data (frame), "remove_node",
                     zframe_size (frame)) == 0) {
                    db_remove_node (update, msg);
                }
                else {
                    if (memcmp
                        (zframe_data (frame), "add_node",
                         zframe_size (frame)) == 0) {
                        db_add_node (update, msg);
                    }
                    else {
                        if (memcmp
                            (zframe_data (frame), "st_piece",
                             zframe_size (frame)) == 0) {
                            db_update_st_piece (update, msg);
                        }
                        else {
                            if (memcmp
                                (zframe_data (frame), "n_pieces",
                                 zframe_size (frame)) == 0) {
                                db_update_n_pieces (update, msg);
                            }
                            else {
                                if (memcmp
                                    (zframe_data (frame), "go_online",
                                     zframe_size (frame)) == 0) {
                                    zmsg_destroy (&msg);

                                }




                            }
                        }
                    }
                }
            }
        }


        zframe_destroy (&frame);


        zframe_send (&id, update->dealer, 0);
        fprintf (stderr, "\nworker_update:I have sent confirmation to sub msg");

    }



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

    zframe_t *db = zmsg_pop (msg);
    if (strcmp ("db", (char *) zframe_data (db)) == 0) {
        zframe_destroy (&db);
        worker_update_db (update, msg);
    }
    else {
        zframe_destroy (&db);

        zframe_t *id = zmsg_pop (msg);
        if (memcmp (zframe_data (id), &(update->id), sizeof (unsigned int)) ==
            0) {
//lazy pirate reconfirm update
            zframe_send (&id, update->dealer, 0);
            zframe_destroy (&id);
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

    char identity[17];

    sprintf (identity, "%swb", worker->id);
    zmq_setsockopt (self_wb, ZMQ_IDENTITY, identity, strlen (identity));
    sprintf (identity, "%snb", worker->id);
    zmq_setsockopt (self_nb, ZMQ_IDENTITY, identity, strlen (identity));

//balance infrastructure

    void *router_bl = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_bl = zsocket_new (ctx, ZMQ_DEALER);
    zmq_setsockopt (self_bl, ZMQ_IDENTITY, worker->id, strlen (worker->id));

//hash of vertices
    khash_t (vertices) * hash = kh_init (vertices);

//sleep object
    sleep_t *sleep;
    sleep_init (&sleep);

//router object
//used to find where each msg goes
    router_t *router;

    router_init (&router, 0);

//router object
//used to find where each msg goes
    router_t *db_router;

    router_init (&db_router, 1);


//balance object
    balance_t *balance;

    balance_init (&balance, hash, router_bl, self_bl, worker->id);

//localdb object
//used to save the counter used to create new vertices
    localdb_t *localdb;
    localdb_init (&localdb, worker->id);

//compute object
    compute_t *compute;

    compute_init (&compute, hash, router, db_router, balance->events,
                  balance->intervals, socket_nb, self_nb, socket_wb, self_wb,
                  localdb, worker);

//update object
//used to update things, like the router object
    update_t *update;


    update_init (&update, dealer, router, db_router, balance, compute);

    zmq_pollitem_t pollitems[4] = { {sub, 0, ZMQ_POLLIN}, {self_bl, 0,
                                                           ZMQ_POLLIN},
    {self_wb, 0,
     ZMQ_POLLIN},
    {self_nb, 0, ZMQ_POLLIN}
    };
    fprintf (stderr, "\nworker with id:%s ready.", worker->id);
//main loop
    while (1) {
        rc = zmq_poll (pollitems, 4, worker_timeout (balance, sleep));
        assert (rc != -1);

        worker_process_timer_events (worker, balance, sleep, compute);

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



int64_t
worker_timeout (balance_t * balance, sleep_t * sleep)
{
//finding the minimum timeout
    int64_t time = zclock_time ();
    int64_t timeout = -1;
    if (balance->next_time < 0) {
        if (sleep->next_time > 0) {

            timeout = time - sleep->next_time;
        }
    }


    else {
        if (sleep->next_time > 0) {
            timeout = time - balance->next_time;
        }
    }

    if (timeout < 0) {
        timeout = -1;
    }

    return timeout;
}


void
worker_process_timer_events (worker_t * worker, balance_t * balance,
                             sleep_t * sleep, compute_t * compute)
{
    int64_t time = zclock_time ();

    if (balance->next_time < time) {
        worker_balance_lazy_pirate (balance);
    }
    if (sleep->next_time < time) {
        worker_sleep (sleep, compute);
    }

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
