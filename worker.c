#include<czmq.h>
#include"worker.h"
#include"worker_update.h"
#include"zookeeper.h"
#include<zookeeper/zookeeper.h>
#include"hash/khash.h"
#include"btree/kbtree.h"
#include"vertex.h"
#include"aknowledgements.h"


int
send (zmsg_t * msg, router_t * router, void *socket_wb, void *socket_nb,
      void *self_wb, void *self_nb)
{
    zframe_t *frame = zmsg_first (msg);
    frame = zframe_next (msg);
    uint64_t key;
    memcpy (&key, zframe_data (frame), sizeof (uint64_t));
    char id[1000];
    router_route (router, key, id);

    //check that self has been initialized
    assert (router->self != NULL);

    //send to self if necessary
    if (strcmp (router->self->key, id) == 0) {
	if (wb) {
	    zmsg_send (&msg, self_wb);
	}
	else {
	    zmsg_send (&msg, self_nb);
	}

    }
    else {
	//write address
	zmsg_wrap (msg, zframe_new (id, strlen (id)));
	if (wb) {
	    zmsg_send (&msg, socket_wb);
	}
	else {
	    zmsg_send (&msg, socket_nb);
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
	char comp_name[1000];
	int buffer;
	int buffer_len = sizeof (int);

	oconfig_octopus (worker->config, octopus);

	sprintf (path, "/%s/global_properties/interval/last_interval",
		 octopus);
	result =
	    zoo_get (worker->zh, path, 0, (char *) buffer, &buffer_len,
		     &stat);
	assert (result == ZOK);

	assert (buffer_len == sizeof (int));

	buffer++;

	result = zoo_set (worker->zh, path, (const char *) buffer,
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


int
update_n_pieces (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    unsigned long n_pieces;
    unsigned long st_piece;
    char bind_point[30];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);
    node_t *prev_node;
    node = node_dup (prev_node = nodes_search (update->router->nodes, key));

    node->n_pieces = n_pieces;

//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 0);

//update router object
//this should always happen after the prev step

    router_delete (update->router, prev_node);
    router_add (update->router, node);

    event_t *event;
    while (event = zlist_pop (events)) {
//check if there is an action that this event describes
	if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
	    if (event_possible (event, update->balance->intervals)) {
//perform this event

//update the intervals

		interval_t *interval;
		interval_init (&interval, event->start event->end);
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
}



int
update_st_piece (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    unsigned long n_pieces;
    unsigned long st_piece;
    char bind_point[30];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);
    node_t *prev_node;
    node = node_dup (prev_node = nodes_search (update->router->nodes, key));

    node->st_piece = st_piece;

//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 0);

//update router object
//this should always happen after the prev step

    router_delete (update->router, prev_node);
    router_add (update->router, node);

    event_t *event;
    while (event = zlist_pop (events)) {
//check if there is an action that this event describes
	if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
	    if (event_possible (event, update->balance->intervals)) {
// perform this event

//update the intervals

		interval_t *interval;
		interval_init (&interval, event->start event->end);
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
}


int
remove_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    unsigned long n_pieces;
    unsigned long st_piece;
    char bind_point[30];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node = nodes_search (update->router->nodes, key);

    assert (node != NULL);

//remove all previous events by this node. they will never happen
//the node is dead

    events_remove (update->balance->events, node);


//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 1);

//update router object
//this should always happen after the prev step
    router_delete (update->router, node);


    event_t *event;
    while (event = zlist_pop (events)) {

	assert (event->give = 0);
//the only possible thing that could happen would be to add 
//the new intervals to this node

	interval_t *interval;
	interval_init (&interval, event->start event->end);

	intervals_add (intervals, interval);
	free (event);




    }


    zlist_destroy (&events);

}









int
add_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    unsigned long n_pieces;
    unsigned long st_piece;
    char bind_point[30];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node_init (&node, key, n_pieces, st_piece, bind_point);

//obtain the new events
    zlist_t *events;
    events = router_events (update->router, node, 0);

//update router object
//this should always happen after the prev step
    if (0 == router_add (update->router, node)) {
	free (node);
    }
    event_t *event;
    while (event = zlist_pop (events)) {
//check if there is an action that this event describes
	if (0 == actions_update (update->balance->actions, event)) {

//check whether we can perform this event now
	    if (event_possible (event, update->balance->intervals)) {
// perform this event

//update the intervals

		interval_t *interval;
		interval_init (&interval, event->start event->end);
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
}


int
add_self (update_t * update, zmsg_t * msg)
{
    node_t *self;
    char key[100];
    unsigned long n_pieces;
    unsigned long st_piece;
    char bind_point[30];

    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node_init (&self, key, n_pieces, st_piece, bind_point);

//update router
    update->router->self = self;

//the rest of the router update will happen when the node goes online

//bind sockets
    rc = zsocket_bind (update->compute->self_nb, "tcp://%s:40002",
		       bind_point);
    assert (rc == 40002);
    rc = zsocket_bind (update->compute->self_wb, "tcp://%s:40003",
		       bind_point);
    assert (rc == 40003);
    rc = zsocket_connect (update->compute->socket_nb, "tcp://%s:40002",
			  bind_point);
    assert (rc == 0);
    rc = zsocket_connect (update->compute->socket_wb, "tcp://%s:40003",
			  bind_point);
    assert (rc == 0);
    rc = zsocket_bind (update->balance->self_bl, "tcp://%s:40004",
		       bind_point);
    assert (rc == 40004);
    rc = zsocket_connect (update->balance->socket_bl, "tcp://%s:40004",
			  bind_point);
    assert (rc == 0);


}

//this sets the node up
//the event of that action hasnt though come from zookeeper
//it doesnt exist in the "view" of this node
int
go_online (worker_t * worker)
{

    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (worker->config, octopus);
    oconfig_octopus (worker->config, comp_name);

    sprintf (path, "/%s/%s/worker_nodes/%s/online", octopus, comp_name,
	     worker->id);

    int result = zoo_create (worker->zh, path, NULL,
			     -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL,
			     0);
    assert (result == ZOK);


}




int
worker_update (update_t * update, void *sub)
{

//check if it is a new update or an old one
    zmsg_t *msg = zmsg_recv (sub);
    zframe_t *id = zframe_pop (msg);
    if (memcmp (zframe_data (id), &(updater->id), sizeof (unsigned int)) == 0) {
//lazy pirate reconfirm update
	zframe_send (id, update->dealer, 0);
	zmsg_destroy (&msg);
    }
    else {
	zframe_t *frame = zframe_pop (msg);
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


	zframe_destroy (frame);


	zframe_send (id, update->dealer, 0);
    }

    return 0;
}


int
sleep_send (sleep_t * sleep, router_t * router, void *socket_wb,
	    void *socket_nb, void *self_wb, void *self_nb)
{
    zmsg_t *msg;
    unsigned short wb;

    while (msg = sleep_awake (sleep, &wb)) {
	send (msg, router, socket_wb, socket_nb, self_wb, self_nb);

    }


}



void
worker_fn (void *arg, zctx_t * ctx, void *pipe)
{

    worker_t *worker = (worker_t *) arg;

    int rc;
//update infrastructure
    void *sub = zsocket_new (ctx, ZMQ_SUB);
    void *dealer = zsocket_new (ctx, ZMQ_DEALER);


    zmq_setsockopt (dealer, ZMQ_IDENTITY, worker->id, strlen (worker->id));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, worker->id, strlen (worker->id));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "all", strlen ("all") + 1);


    rc = zsocket_connect (sub, "tcp://127.0.0.1:40000");
    assert (rc == 0);

    rc = zsocket_connect (dealer, "tcp://127.0.0.1:40001");
    assert (rc == 0);




//worker infrastruct

    void *socket_wb = zsocket_new (ctx, ZMQ_ROUTER);
    void *socket_nb = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_wb = zsocket_new (ctx, ZMQ_DEALER);
    void *self_nb = zsocket_new (ctx, ZMQ_DEALER);

    char *identity = (char *) malloc (1000);

    sprintf (identity, "%swb", worker->id);
    zmq_setsockopt (self_wb, ZMQ_IDENTITY, identity, strlen (identity));
    sprintf (identity, "%snb", worker->id);
    zmq_setsockopt (self_nb, ZMQ_IDENTITY, identity, strlen (identity));

//balance infrastructure

    void *balance = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_bl = zsocket_new (ctx, ZMQ_DEALER);
    zmq_setsockopt (self_bl, ZMQ_IDENTITY, worker->id, strlen (worker->id));

//cleaning
    free (identity);

//tree of vertices
    kbtree_t (vertices) * tree = kb_init (vertices, KB_DEFAULT_SIZE);

//sleep object
    sleep_t *sleep;
    sleep_init (&sleep);

//router object
//used to find where each msg goes
    router_t *router;

    router_init (&router, 0);


//balance object
    balance_t *balance;

    balance_init (balance_t ** balance, tree, router_bl, self_bl, intervals);

//localdb object
//used to save the counter used to create new vertices
    localdb_t *localdb;
    localdb_init (&localdb, worker->id);

//compute object
    compute_t *compute;

    compute_init (compute, hash, socket_nb, self_nb, socket_wb, self_wb,
		  localdb, worker);

//update object
//used to update things, like the router object
    update_t *update;


    update_init (&update, dealer, router, balance, compute);

    zmq_pollitem_t pollitems[4] =
	{ {sub, 0, ZMQ_POLLIN}, {balance, 0, ZMQ_POLLIN}, {router_wb, 0,
							   ZMQ_POLLIN},
    {router_nb, 0, ZMQ_POLLIN}
    };
//main loop
    while (1) {
	rc = zmq_poll (pollitems, 4, sleep->timeout);
	assert (rc != -1);

//sends all msgs that their delay has expired
	sleep_send (sleep, router, socket_wb, socket_nb, self_wb, self_nb);

	if (pollitem[0].revents & ZMQ_POLLIN) {
	    worker_update (update, sub);
	}
	if (pollitem[1].revents & ZMQ_POLLIN) {

	}
	if (pollitem[2].revents & ZMQ_POLLIN) {

	}
	if (pollitem[3].revents & ZMQ_POLLIN) {

	}




    }





}


int
compute_init (conpute_t ** compute, khash_t (vertices) * hash,
	      void *socket_nb, void *self_nb, void *socket_wb, void *self_wb,
	      localdb_t * localdb, worker_t * worker)
{

    *compute = (compute_t *) malloc (sizeof (compute_t));

    compute->socket_nb = socket_nb;
    compute->self_nb = self_nb;
    compute->socket_wb = socket_wb;
    compute->self_wb = self_wb;
    compute->worker = worker;
    compute->hash = hash;
    compute->localdb = localdb;
    compute->interval = localdb_get_interval (localdb);
    compute->counter = localdb_get_counter (localdb);
//in case we just started
    if (compute->interval == -1) {
	compute->interval = worker_new_interval (worker, localdb);
//init counter
	localdb_incr_counter (localdb, 1);
	compute->counter = 1;
    }

//getting the interval size

    int result;
    struct Stat stat;

    char path[1000];
    char octopus[1000];
    char comp_name[1000];
    unsigned long buffer;
    int buffer_len = sizeof (unsigned long);

    oconfig_octopus (worker->config, octopus);

    sprintf (path, "/%s/global_properties/interval/interval_size", octopus);
    result = zoo_get (worker->zh, path, 0, (char *) buffer,
		      &buffer_len, &stat);
    assert (result == ZOK);

    assert (buffer_len == sizeof (unsigned long));

    compute->interval_size = buffer;



}



int
worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config, char *id)
{

    *worker = (worker_t *) malloc (sizeof (worker_t));
    (*worker)->zh = zh;
    (*worker)->id = id;
    (*worker)->config = config;
}

int
workers_init (workers_t ** workers, ozookeeper_t * ozookeeper)
{


    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);

    sprintf (path, "/%s/%s/worker_nodes", octopus, comp_name);
    struct String_vector worker_children;
    result = zoo_get_children (zh, path, 0, &worker_children);


    if (ZOK == result) {

//mallocing
	*workers = (workers_t *) malloc (sizeof (workers_t));
	(*workers)->size = worker_children.count;
	(*workers)->pipe =
	    (void **) malloc (sizeof (void *) * (worker_children.count));
	(*workers)->id =
	    (char **) malloc (sizeof (char *) * (worker_children.count));
//create the threads

	int iter;
	if (worker_children.count < 1000) {
	    worker_t *worker;
	    for (iter = 0; iter < worker_children.count; iter++) {
		(*workers)->id[iter] =
		    (char *) malloc (strlen (worker_children.data[iter]) + 1 +
				     strlen (comp_name));
		sprintf ((*workers)->id[iter], "%s%s", comp_name,
			 worker_children.data[iter]);

		worker_init (&worker, ozookeeper->zh, ozookeeper->config,
			     (*workers)->id[iter]);

		(*workers)->pipe[iter] =
		    zthread_fork (ctx, &worker_fn, worker);
	    }
	}
	else {
	    printf ("\n More workers than allowed.. error exiting");
	    exit (1);
	}
	if (ZOK != result && ZNONODE != result) {
	    printf ("\n Couldnt get the children.. error exiting");
	    exit (1);
	}

    }




    deallocate_String_vector (worker_children);
}
