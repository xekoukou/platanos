#include<zookeeper/zookeeper.h>
#include"config.h"
#include"zookeeper.h"
#include<string.h>
#include<czmq.h>
#include"common.h"
#include"worker.h"

#define RESEND_INTERVAL 1000

void
global_watcherctx_init (global_watcherctx_t ** watcherctx, oconfig_t * config)
{
    *watcherctx =
	(global_watcherctx_t *) malloc (sizeof (global_watcherctx_t));
    (*watcherctx)->retries = 0;
    (*watcherctx)->max_retries = 2000;
    (*watcherctx)->config = config;
}

//initialize the ozookeeper object
void
ozookeeper_init (ozookeeper_t ** ozookeeper, oconfig_t * config,
		 global_watcherctx_t * watcherctx, void *pub, void *router)
{

    *ozookeeper = (ozookeeper_t *) malloc (sizeof (ozookeeper_t));

    (*ozookeeper)->config = config;
    (*ozookeeper)->pub = pub;
    (*ozookeeper)->router = router;
    oz_updater_init (&((*ozookeeper)->updater));


    char host[1000];
    oconfig_host (config, host);
    int recv_timeout;
    oconfig_recv_timeout (config, &recv_timeout);

    watcherctx->ozookeeper = *ozookeeper;

    (*ozookeeper)->zh =
	zookeeper_init (host, global_watcher, recv_timeout, 0, watcherctx, 0);
    if ((*ozookeeper)->zh == NULL) {
	fprintf (stderr, "\nzookeeper client cannot be initialized");
	exit (1);
    }


    if (ozookeeper_not_corrupt (ozookeeper) != 1) {
	fprintf
	    (stderr,
	     "\n local config and zookeeper config do not match,\n Have you registered at least one resource for this computer to zookeeper?\n exiting..");
	exit (1);
    }



}


// 1 for true 0 for false
//checks that the node described in the config exists in the zookeeper
//if not then the config or the zookeeper are corrupt.
int
ozookeeper_not_corrupt (ozookeeper_t ** ozookeep)
{

    ozookeeper_t *ozookeeper = *ozookeep;

    struct Stat stat;
    char path[1000];
    char config[4][1000];

    oconfig_octopus (ozookeeper->config, config[0]);
    oconfig_comp_name (ozookeeper->config, config[1]);

    sprintf (path, "/%s", config[0]);
    if (ZOK != zoo_exists (ozookeeper->zh, path, 0, &stat)) {
	return 0;
    }
    sprintf (path, "/%s/computers/%s", config[0], config[1]);
    if (ZOK != zoo_exists (ozookeeper->zh, path, 0, &stat)) {
	return 0;
    }
    return 1;

}


void
ozookeeper_set_zhandle (ozookeeper_t * ozookeeper, zhandle_t * zh)
{
    ozookeeper->zh = zh;
}

void
ozookeeper_zhandle (ozookeeper_t * ozookeeper, zhandle_t ** zh)
{
    *zh = ozookeeper->zh;
}


void
ozookeeper_destroy (ozookeeper_t * ozookeeper)
{
    zookeeper_close (ozookeeper->zh);
    oconfig_destroy (ozookeeper->config);
    oz_updater_destroy (&(ozookeeper->updater));
    free (ozookeeper);
}

void
global_watcherctx_destroy (global_watcherctx_t * watcherctx)
{
    free (watcherctx);
}

void
ozookeeper_update (ozookeeper_t * ozookeeper, zmsg_t ** msg)
{

    int rc;
    if (ozookeeper->updater.id > 1000000000) {
	ozookeeper->updater.id = 1;
    }
    else {
	ozookeeper->updater.id++;
    };

    zframe_t *address;
    zframe_t *frame;

    zmsg_t *msg_to_send = zmsg_dup (*msg);
    zmsg_push (msg_to_send,
	       zframe_new (&(ozookeeper->updater.id), sizeof (unsigned int)));
    zmsg_push (msg_to_send, zframe_new ("all", strlen ("all") + 1));

    zmsg_send (&msg_to_send, ozookeeper->pub);

    //initializes ok vector
    zlist_t *ok_list = zlist_new ();

    zmsg_t *resp;
    size_t time = zclock_time ();
    zframe_t *iter;

    zmq_pollitem_t pollitems[1] = { {ozookeeper->router, 0, ZMQ_POLLIN} };

    while (1) {
	//TODO the timeout is not the correct
//but it is not of a great deal        
	rc = zmq_poll (pollitems, 1, RESEND_INTERVAL);
	assert (rc != -1);
	if (pollitems[0].revents & ZMQ_POLLIN) {

	    int new = 1;
	    resp = zmsg_recv (ozookeeper->router);
	    address = zmsg_unwrap (resp);
	    frame = zmsg_first (resp);
	    //if correct id and address is new the ok vector is updated
	    if (memcmp
		(zframe_data (frame), &(ozookeeper->updater.id),
		 sizeof (unsigned int)) == 0) {
		iter = zlist_first (ok_list);
		while (iter) {
		    if (zframe_eq (iter, address)) {
			new = 0;
			break;
		    }
		    iter = zlist_next (ok_list);
		}


		if (new) {
		    zlist_append (ok_list, address);
		}
		else {
		    zframe_destroy (&address);
		}
	    }
	    else {

		zframe_destroy (&address);

	    }

	    zmsg_destroy (&resp);

	}

	if (zlist_size (ok_list) == ozookeeper->workers->size) {
//destroy things
	    iter = zlist_first (ok_list);
	    while (iter) {
		zframe_destroy (&iter);
		iter = zlist_next (ok_list);
	    }
	    break;
	}
//send again to those that didnt respond
//it assumes that the address of the dealer is the same with
//the subscription of the sub.
	if (zclock_time () - time > RESEND_INTERVAL) {
	    time = zclock_time ();

	    int it;
	    for (it = 0; it < ozookeeper->workers->size; it++) {
		int exists = 0;
		iter = zlist_first (ok_list);
		while (iter) {
		    if (memcmp
			(zframe_data (address), ozookeeper->workers->id[it],
			 strlen (ozookeeper->workers->id[it])) == 0) {
			exists = 1;
			break;
		    }
		    iter = zlist_next (ok_list);
		}
		if (exists == 0) {
		    msg_to_send = zmsg_dup (*msg);
		    zmsg_push (msg_to_send,
			       zframe_new (&(ozookeeper->updater.id),
					   sizeof (unsigned int)));
		    zmsg_push (msg_to_send,
			       zframe_new (ozookeeper->workers->id[it],
					   strlen (ozookeeper->workers->
						   id[it])));
		    zmsg_send (&msg_to_send, ozookeeper->pub);

		}
	    }

	}

    }
    zmsg_destroy (msg);
}

//init ozookeeper with the workers
void
ozookeeper_init_workers (ozookeeper_t * ozookeeper, workers_t * workers)
{
    ozookeeper->workers = workers;
}

// doesnt allocate memory
void
oz_updater_init (oz_updater_t * updater)
{
    updater->id = 1;
    //allocate_String_vector (&(updater->computers));
    updater->computers.count = 0;
    updater->computers.data = 0;
}

// just creates a path
void
oz_updater_key (oz_updater_t * updater, char *key)
{
    updater->key = (char *) malloc (strlen (key) + 1);
    strcpy (updater->key, key);
}

void
oz_updater_free_key (oz_updater_t * updater)
{
    free (updater->key);
}

void
oz_updater_destroy (oz_updater_t * updater)
{
    free (updater->key);
    int iter;

    for (iter = 0; iter < updater->computers.count; iter++) {
	deallocate_String_vector (&(updater->resources[iter]));
    }
    free (updater->resources);
}

//one node update
void
ozookeeper_update_one (ozookeeper_t * ozookeeper, zmsg_t ** msg)
{
    int rc;
    if (ozookeeper->updater.id > 1000000000) {
	ozookeeper->updater.id = 1;
    }
    else {
	ozookeeper->updater.id++;
    }

    size_t time = zclock_time () - RESEND_INTERVAL;

    zmq_pollitem_t pollitems[1] = { {ozookeeper->router, 0, ZMQ_POLLIN}
    };


    while (1) {
	if (zclock_time () - time > RESEND_INTERVAL) {
	    time = zclock_time ();
	    zmsg_t *msg_to_send = zmsg_dup (*msg);
	    zmsg_push (msg_to_send,
		       zframe_new (&(ozookeeper->updater.id),
				   sizeof (unsigned int)));
	    zmsg_push (msg_to_send,
		       zframe_new (ozookeeper->updater.key,
				   strlen (ozookeeper->updater.key)));
	    zmsg_send (&msg_to_send, ozookeeper->pub);


	}

	rc = zmq_poll (pollitems, 1, RESEND_INTERVAL);
	assert (rc != -1);
	if (pollitems[0].revents & ZMQ_POLLIN) {



	    zmsg_t *resp = zmsg_recv (ozookeeper->router);
	    zframe_t *address = zmsg_unwrap (resp);
	    zframe_t *frame = zmsg_first (resp);
	    //if correct id and address is correct then break
	    if (memcmp
		(zframe_data (frame), &(ozookeeper->updater.id),
		 sizeof (unsigned int)) == 0) {
		if (zframe_streq (address, ozookeeper->updater.key)) {
		    break;
		}
	    }
	    zmsg_destroy (&resp);
	}
    }


    zmsg_destroy (msg);
}

//4 possible update states
//depending on the state, the updater should update its key

//this state happens when a node goes offline
//results are destructive for the data of that node
//st_piece is used to search the node in the router object 

//IMPORTANT st_piece should not be changed when a node is offline
void
ozookeeper_update_remove_node (ozookeeper_t * ozookeeper, char *key)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("remove_node", strlen ("remove_node") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    ozookeeper_update (ozookeeper, &msg);
}

//TODO check
//this is done when a node goes online
void
ozookeeper_update_add_node (ozookeeper_t * ozookeeper, char *key,
			    int n_pieces, unsigned long st_piece,
			    char *bind_point)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("add_node", strlen ("add_node") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    zmsg_add (msg, zframe_new (bind_point, strlen (bind_point) + 1));
    ozookeeper_update (ozookeeper, &msg);
}

//this is done only at the start for a specific number of seconds
void
ozookeeper_update_add_self (ozookeeper_t * ozookeeper, char *key,
			    int n_pieces, unsigned long st_piece,
			    char *bind_point)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("add_self", strlen ("add_self") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    zmsg_add (msg, zframe_new (bind_point, strlen (bind_point) + 1));
    ozookeeper_update_one (ozookeeper, &msg);
}



void
ozookeeper_update_st_piece (ozookeeper_t * ozookeeper, char *key,
			    unsigned long st_piece)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("st_piece", strlen ("st_piece") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    ozookeeper_update (ozookeeper, &msg);
}

void
ozookeeper_update_n_pieces (ozookeeper_t * ozookeeper, char *key,
			    int n_pieces)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("n_pieces", strlen ("n_pieces") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    ozookeeper_update (ozookeeper, &msg);
}

//this is issued to tell the workers to go online
void
ozookeeper_update_go_online (ozookeeper_t * ozookeeper)
{

    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("go_online", strlen ("go_online") + 1));
    ozookeeper_update (ozookeeper, &msg);

}



// these are the watches and completions set by the client
//TODO do the same for the db_nodes

//WATCHERS
void
w_online (zhandle_t * zh, int type,
	  int state, const char *path, void *watcherCtx);

void
w_st_piece (zhandle_t * zh, int type,
	    int state, const char *path, void *watcherCtx)
{


    char spath[1000];

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;
    char octopus[1000];
    char comp_name[1000];
    char resource[1000];
    int result;
    int online = 0;

    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path ((const char *) path, 2, &temp, &temp_size);
    strncpy (resource, temp, temp_size);
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);

    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

//check if node is online TODO remove this by ensuring that watches exist only on online nodes

	struct Stat stat;
	sprintf (spath, "/%s/computers/%s/worker_nodes/%s/online", octopus,
		 comp_name, resource);
	if (ZOK ==
	    (result =
	     zoo_wexists (ozookeeper->zh, spath, w_online, ozookeeper,
			  &stat))) {
	    online = 1;
	}
	else {
	    assert (result == ZNONODE);
	}
	if (online) {
	    int buffer_len = sizeof (unsigned long);
	    unsigned long st_piece;

	    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/st_piece",
		     octopus, comp_name, resource);
	    result =
		zoo_wget (ozookeeper->zh, spath, w_st_piece, ozookeeper,
			  (char *) &st_piece, &buffer_len, &stat);

	    assert (result == ZOK);

	    sprintf (spath, "%s%s", comp_name, resource);
	    ozookeeper_update_st_piece (ozookeeper, spath, st_piece);


	}
    }
}



void
w_n_pieces (zhandle_t * zh, int type,
	    int state, const char *path, void *watcherCtx)
{


    char spath[1000];

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;
    char octopus[1000];
    char comp_name[1000];
    char resource[1000];
    int result;
    int online = 0;

    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path (path, 2, &temp, &temp_size);
    strncpy (resource, temp, temp_size);
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);

    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

//check if node is online TODO remove this by ensuring that watches exist only on online nodes

	struct Stat stat;
	sprintf (spath, "/%s/computers/%s/worker_nodes/%s/online", octopus,
		 comp_name, resource);
	if (ZOK ==
	    (result =
	     zoo_wexists (ozookeeper->zh, spath, w_online, ozookeeper,
			  &stat))) {
	    online = 1;
	}
	else {
	    assert (result == ZNONODE);
	}
	if (online) {
	    int buffer_len = sizeof (int);
	    int n_pieces;

	    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/n_pieces",
		     octopus, comp_name, resource);
	    result =
		zoo_wget (ozookeeper->zh, spath, w_n_pieces, ozookeeper,
			  (char *) &n_pieces, &buffer_len, &stat);

	    assert (result == ZOK);

	    sprintf (spath, "%s%s", comp_name, resource);
	    ozookeeper_update_n_pieces (ozookeeper, spath, n_pieces);


	}
    }
}



void
w_online (zhandle_t * zh, int type,
	  int state, const char *path, void *watcherCtx)
{
    char spath[1000];

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;
    char octopus[1000];
    char comp_name[1000];
    char resource[1000];
    int result;
    int online = 0;

    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path (path, 2, &temp, &temp_size);
    strncpy (resource, temp, temp_size);
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);

    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

//check if this node is online

	struct Stat stat;
	sprintf (spath, "/%s/computers/%s/worker_nodes/%s/online", octopus,
		 comp_name, resource);
	if (ZOK ==
	    (result =
	     zoo_wexists (ozookeeper->zh, spath, w_online, ozookeeper,
			  &stat))) {
	    online = 1;
	}
	else {
	    assert (result == ZNONODE);
	}
	int buffer_len;
	unsigned long st_piece;
	int n_pieces;
	char bind_point[30];


	if (online) {
	    buffer_len = sizeof (int);
	    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/n_pieces",
		     octopus, comp_name, resource);
	    result =
		zoo_wget (ozookeeper->zh, spath, w_n_pieces, ozookeeper,
			  (char *) &n_pieces, &buffer_len, &stat);

	    assert (ZOK == result);

	    buffer_len = sizeof (unsigned int);
	    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/st_piece",
		     octopus, comp_name, resource);
	    result =
		zoo_wget (ozookeeper->zh, spath, w_st_piece, ozookeeper,
			  (char *) &st_piece, &buffer_len, &stat);

	    assert (result == ZOK);

	    buffer_len = 1000;
	    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/bind_point",
		     octopus, comp_name, resource);
	    result =
		zoo_get (ozookeeper->zh, spath, 0, bind_point, &buffer_len,
			 &stat);

	    assert (result == ZOK);



	    sprintf (spath, "%s%s", comp_name, resource);
	    ozookeeper_update_add_node (ozookeeper, spath, n_pieces, st_piece,
					bind_point);
	}
	else {
//if offline delete node if not already deleted
//TODO remove the watches in n_pieces etc. might not required



	    sprintf (spath, "%s%s", comp_name, resource);
	    ozookeeper_update_remove_node (ozookeeper, spath);


	}
    }
}


//update  resources array
//set watches on the online path
//do nothing else
//when the node does go online set watches on the rest
void
w_resources (zhandle_t * zh, int type,
	     int state, const char *path, void *watcherCtx)
{
    int iter;
    int siter;
    char spath[1000];

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;
    char octopus[1000];
    char comp_name[1000];
    int result;
    int online = 0;

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);


    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

	if (strcmp (comp_name, last_path (path)) == 0) {
//do nothing if it is you
	}
	else {

	    struct String_vector resources;

	    result = zoo_wget_children (ozookeeper->zh, path,
					w_resources, ozookeeper, &resources);
	    assert (result == ZOK);

//update resources
//find its location
//at least one computer should exist that has the same name
	    int position = -1;
	    for (iter = 0; iter < ozookeeper->updater.computers.count; iter++) {
		if (strcmp
		    (ozookeeper->updater.computers.data[iter],
		     last_path (path)) == 0) {
		    break;
		    position = iter;
		}
	    }
	    assert (position != -1);

//i use this in case there is a reordering of the existing children
	    int *sort = (int *) malloc (sizeof (int) * resources.count);
//set watches to new computers
	    for (iter = 0; iter < resources.count; iter++) {
		int exists = 0;
		for (siter = 0;
		     siter <
		     ozookeeper->updater.resources[position].count; siter++) {
		    if (strcmp
			(resources.data[iter],
			 ozookeeper->updater.resources[position].
			 data[siter]) == 0) {
			exists = 1;
		    }
		}
		if (exists) {
		    sort[iter] = siter;
		}
		else {
		    sort[iter] = -1;
		}
	    }



	    memcpy (&(ozookeeper->updater.resources[position]),
		    &resources, sizeof (struct String_vector));

	    for (iter = 0; iter < resources.count; iter++) {
//only check the new resources
		if (sort[iter] == -1) {

		    int buffer_len;
		    unsigned long st_piece;
		    int n_pieces;
		    char bind_point[30];
		    struct Stat stat;

//check if this node is online
		    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/online",
			     octopus, last_path (path), resources.data[iter]);
		    if (ZOK ==
			(result =
			 zoo_wexists (ozookeeper->zh, spath, w_online,
				      ozookeeper, &stat))) {
			online = 1;
		    }
		    else {
			assert (result == ZNONODE);
		    }


		    if (online) {

			buffer_len = sizeof (int);
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/n_pieces",
				 octopus, last_path (path),
				 resources.data[iter]);
			result =
			    zoo_wget (ozookeeper->zh, spath, w_n_pieces,
				      ozookeeper, (char *) &n_pieces,
				      &buffer_len, &stat);

			assert (result == ZOK);

			buffer_len = sizeof (unsigned long);
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/st_piece",
				 octopus, last_path (path),
				 resources.data[iter]);
			result =
			    zoo_wget (ozookeeper->zh, spath, w_st_piece,
				      ozookeeper, (char *) &st_piece,
				      &buffer_len, &stat);
			assert (result == ZOK);

			buffer_len = 1000;
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/bind_point",
				 octopus, last_path (path),
				 resources.data[iter]);
			result =
			    zoo_get (ozookeeper->zh, spath, 0, bind_point,
				     &buffer_len, &stat);

			assert (result == ZOK);

			sprintf (spath, "%s%s", last_path (path),
				 resources.data[iter]);
			ozookeeper_update_add_node (ozookeeper, spath,
						    n_pieces, st_piece,
						    bind_point);
		    }
		}
	    }
	}			//TODO check the brackets are correct
    }
}


//set watches on the children of the new computers
void
w_computers (zhandle_t * zh, int type,
	     int state, const char *path, void *watcherCtx)
{
    int iter;
    int siter;
    char spath[1000];
    char octopus[1000];
    int result;
    int online;

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;


    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

	oconfig_octopus (ozookeeper->config, octopus);


	struct String_vector computers;
	struct String_vector resources;

	sprintf (spath, "/%s/computers", octopus);
	result = zoo_wget_children (ozookeeper->zh, spath,
				    w_computers, ozookeeper, &computers);
	assert (result == ZOK);

//i use this in case there is a reordering of the existing children
	int *sort = (int *) malloc (sizeof (int) * computers.count);
//array to free the previous resources
//TODO calloc puts things to zero?
	int size = ozookeeper->updater.computers.count;
	int *array = (int *) calloc (ozookeeper->updater.computers.count,
				     sizeof (int)
	    );

//set watches to new computers
	for (iter = 0; iter < computers.count; iter++) {
	    int exists = 0;
	    for (siter = 0; siter < ozookeeper->updater.computers.count;
		 siter++) {
		if (strcmp
		    (computers.data[iter],
		     ozookeeper->updater.computers.data[siter]) == 0) {
		    exists = 1;
		    break;
		}
	    }
	    if (exists) {
		sort[iter] = siter;
		array[siter] = 1;
	    }
	    else {
		sort[iter] = -1;
	    }
	}
//set the new computer list
	memcpy (&(ozookeeper->updater.computers), &computers,
		sizeof (struct String_vector));
//I allocate the new resources and set watches to new computer's resources
	struct String_vector *new_resources =
	    (struct String_vector *) malloc (sizeof (struct String_vector)
					     * computers.count);
	for (iter = 0; iter < computers.count; iter++) {
	    if (sort[iter] == -1) {

		sprintf (spath, "/%s/computers/%s", octopus,
			 computers.data[iter]);

		result = zoo_wget_children (ozookeeper->zh, spath,
					    w_resources, ozookeeper,
					    &resources);
		assert (result == ZOK);

		for (siter = 0; siter < resources.count; siter++) {



		    int buffer_len;
		    unsigned long st_piece;
		    int n_pieces;
		    char bind_point[30];
		    struct Stat stat;

//check if this node is online
		    sprintf (spath, "/%s/computers/%s/worker_nodes/%s/online",
			     octopus, computers.data[iter],
			     resources.data[siter]);
		    if (ZOK ==
			(result =
			 zoo_wexists (ozookeeper->zh, spath, w_online,
				      ozookeeper, &stat))) {
			online = 1;
		    }
		    else {
			assert (result == ZNONODE);
		    }



		    if (online) {

			sprintf (spath, "%s%s",
				 computers.data[iter], resources.data[siter]);
			oz_updater_free_key (&(ozookeeper->updater));
			oz_updater_key (&(ozookeeper->updater), spath);
			buffer_len = sizeof (int);
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/n_pieces",
				 octopus, computers.data[iter],
				 resources.data[siter]);
			result =
			    zoo_wget (ozookeeper->zh, spath,
				      w_n_pieces, ozookeeper,
				      (char *) &n_pieces, &buffer_len, &stat);

			assert (result == ZOK);

			buffer_len = sizeof (unsigned long);
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/st_piece",
				 octopus, computers.data[iter],
				 resources.data[siter]);
			result =
			    zoo_wget (ozookeeper->zh, spath,
				      w_st_piece, ozookeeper,
				      (char *) &st_piece, &buffer_len, &stat);
			assert (result == ZOK);

			buffer_len = 1000;
			sprintf (spath,
				 "/%s/computers/%s/worker_nodes/%s/bind_point",
				 octopus, computers.data[iter],
				 resources.data[siter]);
			result =
			    zoo_get (ozookeeper->zh, spath, 0,
				     bind_point, &buffer_len, &stat);

			assert (result == ZOK);

			sprintf (spath, "%s%s",
				 computers.data[iter], resources.data[siter]);
			ozookeeper_update_add_node (ozookeeper,
						    spath,
						    n_pieces,
						    st_piece, bind_point);
		    }
		}


		memcpy (&(new_resources[iter]), &resources,
			sizeof (struct String_vector));


	    }
	    else {
		memcpy (&(new_resources[iter]),
			&(ozookeeper->updater.resources[sort[iter]]),
			sizeof (struct String_vector));
	    }
	}
//update the resources and free the previous ones

	for (siter = 0; siter < size; siter++) {
	    if (array[siter] == 0) {
		deallocate_String_vector (&
					  (ozookeeper->updater.
					   resources[siter]));
	    }

	}
	free (ozookeeper->updater.resources);
	ozookeeper->updater.resources = new_resources;
	free (sort);
	free (array);

    }
}

//COMPLETIONS
//TODO check that the breaks lead to the end of the function
void
c_computers (int rc, const struct String_vector *strings, const void *data)
{
    ozookeeper_t *ozookeeper = (ozookeeper_t *) data;
    const struct String_vector *computers = strings;

    char octopus[1000];
    char comp_name[1000];
    int iter;
    int siter;
    char path[1000];
    int result;
    int online = 0;
    int self = 0;

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);


    assert (ZOK == rc);

    memcpy (&(ozookeeper->updater.computers), computers,
	    sizeof (struct String_vector));
    ozookeeper->updater.resources =
	(struct String_vector *) malloc (sizeof (struct String_vector) *
					 computers->count);


    for (siter = 0; siter < computers->count; siter++) {

	struct String_vector resources;
	sprintf (path, "/%s/computers/%s", octopus, computers->data[siter]);

	result = zoo_wget_children (ozookeeper->zh, path,
				    w_resources, ozookeeper, &resources);
	assert (result == ZOK);


	memcpy (&(ozookeeper->updater.resources[siter]), &resources,
		sizeof (struct String_vector));

	for (iter = 0; iter < resources.count; iter++) {



	    int buffer_len;
	    unsigned long st_piece;
	    int n_pieces;
	    char bind_point[30];
	    struct Stat stat;
	    self = 0;

//get data only from nodes that are online or are yourself
	    if (strcmp (computers->data[iter], comp_name) == 0) {
		self = 1;
	    }
//check if this node is online
	    sprintf (path, "/%s/computers/%s/worker_nodes/%s/online",
		     octopus, computers->data[siter], resources.data[iter]);
	    if (ZOK ==
		(result =
		 zoo_wexists (ozookeeper->zh, path, w_online,
			      ozookeeper, &stat))) {
		online = 1;
	    }
	    else {
		if (result != ZNONODE) {
		    break;
		}
	    }

	    if (self || online) {
		buffer_len = sizeof (int);
		sprintf (path, "/%s/computers/%s/worker_nodes/%s/n_pieces",
			 octopus, computers->data[siter],
			 resources.data[iter]);
		result =
		    zoo_wget (ozookeeper->zh, path, w_n_pieces,
			      ozookeeper, (char *) &n_pieces,
			      &buffer_len, &stat);

		assert (result == ZOK);



		buffer_len = sizeof (unsigned long);
		sprintf (path, "/%s/computers/%s/worker_nodes/%s/st_piece",
			 octopus, computers->data[siter],
			 resources.data[iter]);
		result =
		    zoo_wget (ozookeeper->zh, path, w_st_piece,
			      ozookeeper, (char *) &st_piece,
			      &buffer_len, &stat);


		assert (result == ZOK);

		buffer_len = 1000;
		sprintf (path,
			 "/%s/computers/%s/worker_nodes/%s/bind_point",
			 octopus, computers->data[siter],
			 resources.data[iter]);
		result =
		    zoo_get (ozookeeper->zh, path, 0, bind_point,
			     &buffer_len, &stat);

		assert (result == ZOK);



	    }

	    sprintf (path, "%s%s", computers->data[siter],
		     resources.data[iter]);
	    if (self) {
		oz_updater_free_key (&(ozookeeper->updater));
		oz_updater_key (&(ozookeeper->updater), path);
		ozookeeper_update_add_self (ozookeeper, path,
					    n_pieces, st_piece, bind_point);
	    }
//self cannot be online at this point anyway
	    else {
		if (online) {

		    ozookeeper_update_add_node (ozookeeper, path,
						n_pieces,
						st_piece, bind_point);
		}

	    }

	    //TODO check the brackets are correct
	}
    }

    ozookeeper_update_go_online (ozookeeper);

}


//one time function that sets watches on the nodes
//and gets configuration
void
ozookeeper_getconfig (ozookeeper_t * ozookeeper)
{
//if we are in a session expired state or auth failure, global watcher will restart things
// maybe we ll get caught in the middle of working in this, so 
//we need to delete things in the worker and exit

//get worker nodes of this computer

    int result;
    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);

//initialize self
    sprintf (path, "/%s/computers", octopus);
    result =
	zoo_awget_children (ozookeeper->zh, path, w_computers, ozookeeper,
			    c_computers, ozookeeper);
    assert (ZOK == result);

}


static const char *
state2String (int state)
{
    if (state == 0)
	return "CLOSED_STATE";
    if (state == ZOO_CONNECTING_STATE)
	return "CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)
	return "ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)
	return "CONNECTED_STATE";
    if (state == ZOO_EXPIRED_SESSION_STATE)
	return "EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)
	return "AUTH_FAILED_STATE";

    return "INVALID_STATE";
}

static const char *
type2String (int state)
{
    if (state == ZOO_CREATED_EVENT)
	return "CREATED_EVENT";
    if (state == ZOO_DELETED_EVENT)
	return "DELETED_EVENT";
    if (state == ZOO_CHANGED_EVENT)
	return "CHANGED_EVENT";
    if (state == ZOO_CHILD_EVENT)
	return "CHILD_EVENT";
    if (state == ZOO_SESSION_EVENT)
	return "SESSION_EVENT";
    if (state == ZOO_NOTWATCHING_EVENT)
	return "NOTWATCHING_EVENT";

    return "UNKNOWN_EVENT_TYPE";
}


void
global_watcher (zhandle_t * zzh, int type, int state, const char *path,
		void *context)
{

    global_watcherctx_t *watcherctx = (global_watcherctx_t *) context;

    fprintf (stderr, "Watcher %s state = %s", type2String (type),
	     state2String (state));
    if (path && strlen (path) > 0) {
	fprintf (stderr, " for path %s", path);
    }
    fprintf (stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
	if (state == ZOO_CONNECTED_STATE) {
	    watcherctx->retries = 0;
	    fprintf (stderr, "(Re)connected with session id: 0x%llx\n",
		     _LL_CAST_ (zoo_client_id (zzh))->client_id);
	}
    }
    else {
	if (watcherctx->retries < watcherctx->max_retries) {
	    if (state == ZOO_AUTH_FAILED_STATE) {
		fprintf (stderr, "Authentication failure. Retrying...\n");
		watcherctx->retries++;
		ozookeeper_destroy (watcherctx->ozookeeper);
		ozookeeper_init (&(watcherctx->ozookeeper),
				 watcherctx->config, watcherctx,
				 watcherctx->ozookeeper->pub,
				 watcherctx->ozookeeper->router);

	    }
	    else if (state == ZOO_EXPIRED_SESSION_STATE) {
		fprintf (stderr, "Session expired. Retrying...\n");
		watcherctx->retries++;
		ozookeeper_destroy (watcherctx->ozookeeper);
		ozookeeper_init (&(watcherctx->ozookeeper),
				 watcherctx->config, watcherctx,
				 watcherctx->ozookeeper->pub,
				 watcherctx->ozookeeper->router);

	    }
	}
	else {
	    fprintf (stderr, "Maximum retries reached. Shutting down...\n");
	    ozookeeper_destroy (watcherctx->ozookeeper);
	    global_watcherctx_destroy (watcherctx);
	}
    }
}
