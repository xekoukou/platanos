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



#include<zookeeper/zookeeper.h>
#include"config.h"
#include"zookeeper.h"
#include<string.h>
#include<czmq.h>
#include"common.h"
#include"worker.h"
#include"db.h"
#include<stdlib.h>

#define RESEND_INTERVAL 1000


//initialize the ozookeeper object
void
ozookeeper_init (ozookeeper_t ** ozookeeper, oconfig_t * config,
		 void *w_pub, void *w_router, void *db_pub, void *db_router)
{
//USED TO DEBUG ,revert
//zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);


    *ozookeeper = malloc (sizeof (ozookeeper_t));

    (*ozookeeper)->config = config;
    (*ozookeeper)->w_pub = w_pub;
    (*ozookeeper)->w_router = w_router;
    (*ozookeeper)->db_pub = db_pub;
    (*ozookeeper)->db_router = db_router;

    oz_updater_init (&((*ozookeeper)->updater));


    char host[1000];
    oconfig_host (config, host);
    int recv_timeout;
    oconfig_recv_timeout (config, &recv_timeout);


    (*ozookeeper)->zh =
	zookeeper_init (host, global_watcher, recv_timeout, 0, NULL, 0);
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

//TODO referencing dbs_t as workers_t
void
ozookeeper_update (ozookeeper_t * ozookeeper, zmsg_t ** msg, int db)
{
    void *pub;
    void *router;

    workers_t *thread_list;
    if (db) {
	pub = ozookeeper->db_pub;
	router = ozookeeper->db_router;
	thread_list = (workers_t *) ozookeeper->dbs;
    }
    else {
	pub = ozookeeper->w_pub;
	router = ozookeeper->w_router;
	thread_list = ozookeeper->workers;
    }




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

    zmsg_send (&msg_to_send, pub);

    fprintf (stderr, "\nzookeeper_update: I Have sent a sub msg");

    //initializes ok vector
    zlist_t *ok_list = zlist_new ();

    zmsg_t *resp;
    size_t time = zclock_time ();
    zframe_t *iter;

    zmq_pollitem_t pollitems[1] = { {router, 0, ZMQ_POLLIN} };

    while (1) {
	//TODO the timeout is not the correct
//but it is not of a great deal        
	rc = zmq_poll (pollitems, 1, RESEND_INTERVAL);
	assert (rc != -1);
	if (pollitems[0].revents & ZMQ_POLLIN) {

	    int new = 1;
	    resp = zmsg_recv (router);

	    if (!resp) {
		exit (1);
	    }
	    fprintf (stderr,
		     "\nzookeeper_update: I have received a confirmation msg");

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


	if (zlist_size (ok_list) == thread_list->size) {
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
	    for (it = 0; it < thread_list->size; it++) {
		int exists = 0;
		iter = zlist_first (ok_list);
		while (iter) {
		    if (memcmp
			(zframe_data (address), thread_list->id[it],
			 strlen (thread_list->id[it])) == 0) {
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
			       zframe_new (thread_list->id[it],
					   strlen (thread_list->id[it])));
		    zmsg_send (&msg_to_send, pub);

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

//init ozookeeper with the workers
void
ozookeeper_init_dbs (ozookeeper_t * ozookeeper, dbs_t * dbs)
{
    ozookeeper->dbs = dbs;
}



// doesnt allocate memory
void
oz_updater_init (oz_updater_t * updater)
{
    updater->id = 1;
    //allocate_String_vector (&(updater->computers));
    updater->computers.count = 0;
    updater->computers.data = 0;
    updater->w_resources = NULL;
    updater->w_online = NULL;
    updater->db_resources = NULL;
    updater->db_online = NULL;
    updater->key = NULL;
}

// just creates a path
void
oz_updater_key (oz_updater_t * updater, char *key)
{
    updater->key = malloc (strlen (key) + 1);
    strcpy (updater->key, key);
}

void
oz_updater_free_key (oz_updater_t * updater)
{
    if (updater->key != NULL) {
	free (updater->key);
    }
}

void
oz_updater_destroy (oz_updater_t * updater)
{
    free (updater->key);
    int iter;

    for (iter = 0; iter < updater->computers.count; iter++) {
	deallocate_String_vector (&(updater->w_resources[iter]));
	deallocate_String_vector (&(updater->db_resources[iter]));
	free (updater->w_online[iter]);
	free (updater->db_online[iter]);
    }
    free (updater->w_resources);
    free (updater->db_resources);
    free (updater->db_online);
    free (updater->w_online);
}

//returns the location where the resource is ex.(5, 17)
//if it doesnt exist you get (-1,something) or (something,-1)
void
oz_updater_search (oz_updater_t * updater, int db, char *comp_name,
		   char *res_name, int *m, int *n)
{
    *m = -1;
    *n = -1;

    int iter;
    for (iter = 0; iter < updater->computers.count; iter++) {
	if (strcmp (updater->computers.data[iter], comp_name) == 0) {
	    *m = iter;
	    break;
	}
    }
    if (db) {
	for (iter = 0; iter < updater->db_resources[*m].count; iter++) {
	    if (strcmp (updater->db_resources[*m].data[iter], res_name) == 0) {
		*n = iter;
		break;
	    }
	}
    }
    else {
	for (iter = 0; iter < updater->w_resources[*m].count; iter++) {
	    if (strcmp (updater->w_resources[*m].data[iter], res_name) == 0) {
		*n = iter;
		break;
	    }
	}




    }

}


//one node update
void
ozookeeper_update_one (ozookeeper_t * ozookeeper, zmsg_t ** msg, int db)
{
    void *pub;
    void *router;
    if (db) {
	pub = ozookeeper->db_pub;
	router = ozookeeper->db_router;
    }
    else {
	pub = ozookeeper->w_pub;
	router = ozookeeper->w_router;
    }

    fprintf (stderr, "\nrecipient resource:%s", ozookeeper->updater.key);


    int rc;
    if (ozookeeper->updater.id > 1000000000) {
	ozookeeper->updater.id = 1;
    }
    else {
	ozookeeper->updater.id++;
    }

    size_t time = zclock_time () - RESEND_INTERVAL;

    zmq_pollitem_t pollitems[1] = { {router, 0, ZMQ_POLLIN}
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
	    zmsg_send (&msg_to_send, pub);

	    fprintf (stderr, "\nzookeeper_update_one: I Have sent a sub msg");


	}

	rc = zmq_poll (pollitems, 1, RESEND_INTERVAL);
	assert (rc != -1);
	if (pollitems[0].revents & ZMQ_POLLIN) {

	    fprintf (stderr,
		     "\nzookeeper_update_one: I have received a confirmation msg");

	    zmsg_t *resp = zmsg_recv (router);
	    if (!resp) {
		exit (1);
	    }

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
ozookeeper_update_remove_node (ozookeeper_t * ozookeeper, int db, char *key)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("remove_node", strlen ("remove_node") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    ozookeeper_update (ozookeeper, &msg, db);
}

//this is used only by the database
void
ozookeeper_update_delete_node (ozookeeper_t * ozookeeper, char *key)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("delete_node", strlen ("delete_node") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    ozookeeper_update (ozookeeper, &msg, 1);
}



//TODO check
//this is done when a node goes online
void
ozookeeper_update_add_node (ozookeeper_t * ozookeeper, int db, int start,
			    char *key, int n_pieces, unsigned long st_piece,
			    char *bind_point_nb, char *bind_point_wb,
			    char *bind_point_bl)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("add_node", strlen ("add_node") + 1));
    zmsg_add (msg, zframe_new (&start, sizeof (int)));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    zmsg_add (msg, zframe_new (bind_point_nb, strlen (bind_point_nb) + 1));
    if (!db) {
	zmsg_add (msg,
		  zframe_new (bind_point_wb, strlen (bind_point_wb) + 1));
	zmsg_add (msg,
		  zframe_new (bind_point_bl, strlen (bind_point_bl) + 1));
    }

    fprintf (stderr,
	     "\nzookeeper_add_node\nkey:%s\nn_pieces:%d\nst_piece:%lu", key,
	     n_pieces, st_piece);
    zclock_sleep (1000);
    ozookeeper_update (ozookeeper, &msg, db);
}

//this is done only at the start for a specific number of seconds
void
ozookeeper_update_add_self (ozookeeper_t * ozookeeper, int db, char *key,
			    int n_pieces, unsigned long st_piece,
			    char *bind_point_nb, char *bind_point_wb,
			    char *bind_point_bl)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("add_self", strlen ("add_self") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    zmsg_add (msg, zframe_new (bind_point_nb, strlen (bind_point_nb) + 1));
    if (!db) {
	zmsg_add (msg,
		  zframe_new (bind_point_wb, strlen (bind_point_wb) + 1));
	zmsg_add (msg,
		  zframe_new (bind_point_bl, strlen (bind_point_bl) + 1));
    }
    fprintf (stderr,
	     "\nzookeeper_add_self\nkey:%s\nn_pieces:%d\nst_piece:%lu", key,
	     n_pieces, st_piece);
    ozookeeper_update_one (ozookeeper, &msg, db);
}



void
ozookeeper_update_st_piece (ozookeeper_t * ozookeeper, int db, char *key,
			    unsigned long st_piece)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("st_piece", strlen ("st_piece") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&st_piece, sizeof (unsigned long)));
    ozookeeper_update (ozookeeper, &msg, db);
}

void
ozookeeper_update_n_pieces (ozookeeper_t * ozookeeper, int db, char *key,
			    int n_pieces)
{
    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("n_pieces", strlen ("n_pieces") + 1));
    zmsg_add (msg, zframe_new (key, strlen (key) + 1));
    zmsg_add (msg, zframe_new (&n_pieces, sizeof (int)));
    ozookeeper_update (ozookeeper, &msg, db);
}

//this is issued to tell the workers to go online
void
ozookeeper_update_go_online (ozookeeper_t * ozookeeper, int db)
{

    fprintf (stderr,
	     "\ndb:%d zookeeper_go_online: I am asking the threads to go online",
	     db);

    zmsg_t *msg = zmsg_new ();
    zmsg_add (msg, zframe_new ("go_online", strlen ("go_online") + 1));
    ozookeeper_update (ozookeeper, &msg, db);

}


//watcher helper functions

void
w_st_piece (zhandle_t * zh, int type,
	    int state, const char *path, void *watcherCtx);


void
w_n_pieces (zhandle_t * zh, int type,
	    int state, const char *path, void *watcherCtx);

void
online (ozookeeper_t * ozookeeper, int db, int online, int start, int self,
	char *comp_name, char *res_name)
{
    char octopus[1000];
    char path[1000];
    char type[15];
    int result;
    if (db) {
	strcpy (type, "db_nodes");
    }
    else {
	strcpy (type, "worker_nodes");
    }

    oconfig_octopus (ozookeeper->config, octopus);

    int buffer_len;
    unsigned long st_piece;
    int n_pieces;
    char bind_point_nb[50];
    char bind_point_wb[50];
    char bind_point_bl[50];
    struct Stat stat;


    if (online) {
//obtain the resources and send them while setting wathcers on st_piece and n_pieces
//we dont need to check if they exist since a resource goes online only after we have registered it.
//we have to set the resource offline before we unregister it.


	buffer_len = sizeof (int);
	sprintf (path, "/%s/computers/%s/%s/%s/n_pieces",
		 octopus, comp_name, type, res_name);
	result =
	    zoo_wget (ozookeeper->zh, path, w_n_pieces, ozookeeper,
		      (char *) &n_pieces, &buffer_len, &stat);

	assert (ZOK == result);

	buffer_len = sizeof (unsigned long);
	sprintf (path, "/%s/computers/%s/%s/%s/st_piece",
		 octopus, comp_name, type, res_name);
	result =
	    zoo_wget (ozookeeper->zh, path, w_st_piece, ozookeeper,
		      (char *) &st_piece, &buffer_len, &stat);

	assert (result == ZOK);

	if (db) {

	    buffer_len = 1000;
	    sprintf (path, "/%s/computers/%s/db_nodes/%s/bind_point",
		     octopus, comp_name, res_name);
	    result =
		zoo_get (ozookeeper->zh, path, 0, bind_point_nb, &buffer_len,
			 &stat);

	    assert (result == ZOK);

	}
	else {

	    buffer_len = 1000;
	    sprintf (path, "/%s/computers/%s/worker_nodes/%s/bind_point_nb",
		     octopus, comp_name, res_name);
	    result =
		zoo_get (ozookeeper->zh, path, 0, bind_point_nb, &buffer_len,
			 &stat);

	    assert (result == ZOK);

	    buffer_len = 1000;
	    sprintf (path, "/%s/computers/%s/worker_nodes/%s/bind_point_wb",
		     octopus, comp_name, res_name);
	    result =
		zoo_get (ozookeeper->zh, path, 0, bind_point_wb, &buffer_len,
			 &stat);

	    assert (result == ZOK);
	    buffer_len = 1000;
	    sprintf (path, "/%s/computers/%s/worker_nodes/%s/bind_point_bl",
		     octopus, comp_name, res_name);
	    result =
		zoo_get (ozookeeper->zh, path, 0, bind_point_bl, &buffer_len,
			 &stat);

	    assert (result == ZOK);

	}
    }

    sprintf (path, "%s%s", comp_name, res_name);


    if (self) {
	assert (online == 1);
	oz_updater_free_key (&(ozookeeper->updater));
	oz_updater_key (&(ozookeeper->updater), path);

	ozookeeper_update_add_self (ozookeeper, db, path, n_pieces, st_piece,
				    bind_point_nb, bind_point_wb,
				    bind_point_bl);


    }
    else {

//update its status on the updater object
	int m;
	int n;
	oz_updater_search (&(ozookeeper->updater), db, comp_name, res_name,
			   &m, &n);
	assert (m != -1);
	assert (n != -1);


	if (online) {

	    ozookeeper_update_add_node (ozookeeper, db, start, path, n_pieces,
					st_piece, bind_point_nb,
					bind_point_wb, bind_point_bl);

	    if (db) {
		ozookeeper->updater.db_online[m][n] = 1;
	    }
	    else {
		ozookeeper->updater.w_online[m][n] = 1;
	    }
	}
	else {
//this is the case where it was previously online but now it is offline
	    if (db) {
		if (ozookeeper->updater.db_online[m][n] == 1) {
		    ozookeeper->updater.db_online[m][n] = 0;
		    ozookeeper_update_remove_node (ozookeeper, db, path);
		}
	    }
	    else {
		if (ozookeeper->updater.w_online[m][n] == 1) {
		    ozookeeper->updater.w_online[m][n] = 0;
		    ozookeeper_update_remove_node (ozookeeper, db, path);
		}

	    }
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
    char res_name[1000];
    char kind[1000];
    int db;
    int result;

    // get the options
    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path (path, 2, &temp, &temp_size);
    strncpy (res_name, temp, temp_size);
    res_name[temp_size] = '\0';
    part_path (path, 3, &temp, &temp_size);
    strncpy (kind, temp, temp_size);
    kind[temp_size] = '\0';
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);
    comp_name[temp_size] = '\0';

    if (strcmp (kind, "worker_nodes") == 0) {
	db = 0;

    }
    else {

	db = 1;
    }

    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {


//check if this node is online

	struct Stat stat;
	sprintf (spath, "/%s/computers/%s/%s/%s/online", octopus,
		 comp_name, kind, res_name);
	if (ZOK ==
	    (result =
	     zoo_wexists (ozookeeper->zh, spath, w_online, ozookeeper,
			  &stat))) {
	    online (ozookeeper, db, 1, 0, 0, comp_name, res_name);
	}
	else {
	    assert (result == ZNONODE);
	    online (ozookeeper, db, 0, 0, 0, comp_name, res_name);
	}

    }
}




//both w_st_piece and w_n_piece need to check the status of the node whether it is
//online. They cant ask the zookeeper because then the order of events would not be preserved
//as is the order of watchers


void
w_st_piece (zhandle_t * zh, int type,
	    int state, const char *path, void *watcherCtx)
{

    char spath[1000];

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;
    char octopus[1000];
    char comp_name[1000];
    char res_name[1000];
    char kind[1000];
    int result;
    int online = 0;
    int db;

    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path ((const char *) path, 2, &temp, &temp_size);
    strncpy (res_name, temp, temp_size);
    res_name[temp_size] = '\0';
    part_path (path, 3, &temp, &temp_size);
    strncpy (kind, temp, temp_size);
    kind[temp_size] = '\0';
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);
    comp_name[temp_size] = '\0';


    if (strcmp (kind, "worker_nodes") == 0) {
	db = 0;

    }
    else {

	db = 1;
    }



    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

//check if the node is online
	int m;
	int n;
	oz_updater_search (&(ozookeeper->updater), db, comp_name, res_name,
			   &m, &n);
	assert (m != -1);
	assert (n != -1);
	if (db) {
	    online = ozookeeper->updater.db_online[m][n];
	}
	else {
	    online = ozookeeper->updater.w_online[m][n];
	}

	if (online) {

//check if it exists(an unregister event might be happening
//thus we dont need a watcher for the existence

	    struct Stat stat;



	    if (ZOK == (result = zoo_exists (ozookeeper->zh, path, 0, &stat))) {

		int buffer_len = sizeof (unsigned long);
		unsigned long st_piece;

		sprintf (spath, "/%s/computers/%s/worker_nodes/%s/st_piece",
			 octopus, comp_name, res_name);
		result =
		    zoo_wget (ozookeeper->zh, spath, w_st_piece, ozookeeper,
			      (char *) &st_piece, &buffer_len, &stat);

		assert (result == ZOK);

		sprintf (spath, "%s%s", comp_name, res_name);
		ozookeeper_update_st_piece (ozookeeper, db, spath, st_piece);




	    }
	    else {
		assert (result == ZNONODE);
	    }




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
    char res_name[1000];
    char kind[1000];
    int result;
    int online = 0;
    int db;

    oconfig_octopus (ozookeeper->config, octopus);
    char *temp;
    int temp_size;
    part_path ((const char *) path, 2, &temp, &temp_size);
    strncpy (res_name, temp, temp_size);
    res_name[temp_size] = '\0';
    part_path (path, 3, &temp, &temp_size);
    strncpy (kind, temp, temp_size);
    kind[temp_size] = '\0';
    part_path (path, 4, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);
    comp_name[temp_size] = '\0';


    if (strcmp (kind, "worker_nodes") == 0) {
	db = 0;

    }
    else {

	db = 1;
    }



    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

//check if the node is online
	int m;
	int n;
	oz_updater_search (&(ozookeeper->updater), db, comp_name, res_name,
			   &m, &n);
	assert (m != -1);
	assert (n != -1);

	if (db) {
	    online = ozookeeper->updater.db_online[m][n];
	}
	else {
	    online = ozookeeper->updater.w_online[m][n];

	}

	if (online) {

//check if it exists(an unregister event might be happening
//thus we dont need a watcher for the existence

	    struct Stat stat;



	    if (ZOK == (result = zoo_exists (ozookeeper->zh, path, 0, &stat))) {

		int buffer_len = sizeof (int);
		int n_pieces;

		sprintf (spath, "/%s/computers/%s/worker_nodes/%s/n_pieces",
			 octopus, comp_name, res_name);
		result =
		    zoo_wget (ozookeeper->zh, spath, w_n_pieces, ozookeeper,
			      (char *) &n_pieces, &buffer_len, &stat);

		assert (result == ZOK);

		sprintf (spath, "%s%s", comp_name, res_name);
		ozookeeper_update_n_pieces (ozookeeper, db, spath, n_pieces);




	    }
	    else {
		assert (result == ZNONODE);
	    }




	}







    }
}

void
w_resources (zhandle_t * zh, int type,
	     int state, const char *path, void *watcherCtx);



//update  resources array
//set watches on the online path
//do nothing else
//when the node does go online set watches on the rest

//the start flag is used to show whether the fucntion is used in the initial configuration or
//in a w_resources function
void
resources (ozookeeper_t * ozookeeper, char *path, int start)
{
    int iter;
    int siter;
    char spath[1000];

    char octopus[1000];
    char comp_name[1000];
    char my_comp_name[1000];
    int result;
    int onlin = 0;
    int self = 0;
    int db;


    char kind[1000];

    char *temp;
    int temp_size;
    part_path (path, 2, &temp, &temp_size);
    strncpy (comp_name, temp, temp_size);
    comp_name[temp_size] = '\0';
    part_path (path, 1, &temp, &temp_size);
    strncpy (kind, temp, temp_size);
    kind[temp_size] = '\0';

    if (strcmp (kind, "worker_nodes") == 0) {
	db = 0;

    }
    else {

	db = 1;
    }




    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, my_comp_name);



    if (strcmp (my_comp_name, comp_name) == 0) {
//you are not supposed to change the resources when online
//exit
	if (!start) {
	    return;
	}
	self = 1;
    }

    //check if it exists

    struct Stat stat;


    result = zoo_wexists (ozookeeper->zh, path,
			  w_resources, ozookeeper, &stat);
    if (result == ZOK) {




	struct String_vector resources;

	result = zoo_wget_children (ozookeeper->zh, path,
				    w_resources, ozookeeper, &resources);

	if (result != ZNONODE) {


	    assert (result == ZOK);

	    fprintf (stderr, "\ndb:%d registered resources for computer %s",
		     db, comp_name);
	    for (iter = 0; iter < resources.count; iter++) {
		fprintf (stderr, "\n%s", resources.data[iter]);
	    }







//update resources
//find its location
//at least one computer should exist that has the same name
	    int position = -1;
	    for (iter = 0; iter < ozookeeper->updater.computers.count; iter++) {
		if (strcmp
		    (ozookeeper->updater.computers.data[iter],
		     comp_name) == 0) {
		    position = iter;
		    break;
		}
	    }
	    assert (position != -1);



	    struct String_vector *old_resources;
	    int **old_online;

	    if (db) {
		old_resources = &(ozookeeper->updater.db_resources[position]);
		old_online = ozookeeper->updater.db_online;
	    }
	    else {
		old_resources = &(ozookeeper->updater.w_resources[position]);
		old_online = ozookeeper->updater.w_online;

	    }


//i use this in case there is a reordering of the existing children
	    int *sort = malloc (sizeof (int) * resources.count);
//update the online vector
	    int *online_vector =
		(int *) calloc (resources.count, sizeof (int));

//set watches to new resources
	    for (iter = 0; iter < resources.count; iter++) {
		int exists = 0;
		for (siter = 0; siter < old_resources->count; siter++) {
		    if (strcmp
			(resources.data[iter],
			 old_resources->data[siter]) == 0) {
			exists = 1;
		    }
		}
		if (exists) {
		    sort[iter] = siter;
		    online_vector[iter] = old_online[position][siter];
		}
		else {
		    sort[iter] = -1;
//databases need to know whether a node has unregistered
		    if (db) {
			sprintf (path, "%s%s",
				 ozookeeper->updater.computers.data[position],
				 old_resources->data[siter]);
			ozookeeper_update_delete_node (ozookeeper, spath);

		    }
		}
	    }

	    deallocate_String_vector (old_resources);


	    memcpy (old_resources, &resources, sizeof (struct String_vector));
	    if (old_online[position] != NULL) {
		free (old_online[position]);
	    }
	    old_online[position] = online_vector;


	    fprintf (stderr, "\ndb:%d new resources for computer %s", db,
		     comp_name);
	    for (iter = 0; iter < resources.count; iter++) {
		if (sort[iter] == -1) {
		    fprintf (stderr, "\n%s", resources.data[iter]);
		}
	    }

	    //check the new resources

	    for (iter = 0; iter < resources.count; iter++) {
		if (sort[iter] == -1) {


//check if this node is online
		    sprintf (spath, "/%s/computers/%s/%s/%s/online",
			     octopus, comp_name, kind, resources.data[iter]);
		    if (ZOK ==
			(result =
			 zoo_wexists (ozookeeper->zh, spath, w_online,
				      ozookeeper, &stat))) {
			onlin = 1;
		    }
		    else {
			assert (result == ZNONODE);
			onlin = 0;
		    }
		    if (self) {
			onlin = 1;
		    }
		    online (ozookeeper, db, onlin, start, self, comp_name,
			    resources.data[iter]);

		}
	    }
	}
    }
}


void
w_resources (zhandle_t * zh, int type,
	     int state, const char *path, void *watcherCtx)
{

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;


    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

	resources (ozookeeper, (char *) path, 0);


    }

}

void w_computers (zhandle_t * zh, int type,
		  int state, const char *path, void *watcherCtx);


void
computers (ozookeeper_t * ozookeeper, int start)
{


    int iter;
    int siter;
    char spath[1000];
    char octopus[1000];
    int result;

    oconfig_octopus (ozookeeper->config, octopus);


    struct String_vector computers;

    sprintf (spath, "/%s/computers", octopus);
    result = zoo_wget_children (ozookeeper->zh, spath,
				w_computers, ozookeeper, &computers);
    assert (result == ZOK);

//i use this in case there is a reordering of the existing children
    int *sort = malloc (sizeof (int) * computers.count);
//array to free the previous resources
//TODO calloc puts things to zero?
    int size = ozookeeper->updater.computers.count;
    int *array = (int *) calloc (ozookeeper->updater.computers.count,
				 sizeof (int)
	);

//set watches to new computers
    for (iter = 0; iter < computers.count; iter++) {
	int exists = 0;
	for (siter = 0; siter < ozookeeper->updater.computers.count; siter++) {
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

    deallocate_String_vector (&(ozookeeper->updater.computers));

//set the new computer list
    memcpy (&(ozookeeper->updater.computers), &computers,
	    sizeof (struct String_vector));


//I allocate the new resources and set watches to new computer's resources
    struct String_vector *new_w_resources =
	malloc (sizeof (struct String_vector)
		* computers.count);

    int **new_w_online_matrix = malloc (sizeof (int *) * computers.count);

    struct String_vector *new_db_resources =
	malloc (sizeof (struct String_vector)
		* computers.count);

    int **new_db_online_matrix = malloc (sizeof (int *) * computers.count);


//obtain the previous data
    for (iter = 0; iter < computers.count; iter++) {
	if (sort[iter] != -1) {

	    memcpy (&(new_w_resources[iter]),
		    &(ozookeeper->updater.w_resources[sort[iter]]),
		    sizeof (struct String_vector));
	    new_w_online_matrix[iter] =
		ozookeeper->updater.w_online[sort[iter]];

	    memcpy (&(new_db_resources[iter]),
		    &(ozookeeper->updater.db_resources[sort[iter]]),
		    sizeof (struct String_vector));
	    new_db_online_matrix[iter] =
		ozookeeper->updater.db_online[sort[iter]];


	}
	else {
//initialize the vector
	    new_w_resources[iter].count = 0;
	    new_w_resources[iter].data = 0;
	    new_w_online_matrix[iter] = NULL;

	    new_db_resources[iter].count = 0;
	    new_db_resources[iter].data = 0;
	    new_db_online_matrix[iter] = NULL;


	}
    }

//free the previous memory and update the pointer to the new memory

    for (siter = 0; siter < size; siter++) {
	if (array[siter] == 0) {
	    deallocate_String_vector (&
				      (ozookeeper->updater.
				       w_resources[siter]));
	    free (ozookeeper->updater.w_online[siter]);

	    deallocate_String_vector (&
				      (ozookeeper->updater.
				       db_resources[siter]));
	    free (ozookeeper->updater.db_online[siter]);

	}

    }
    if (ozookeeper->updater.w_resources != NULL) {
	free (ozookeeper->updater.w_resources);
	assert (ozookeeper->updater.w_online != NULL);
	assert (ozookeeper->updater.db_resources != NULL);
	assert (ozookeeper->updater.db_online != NULL);
	free (ozookeeper->updater.w_online);

	free (ozookeeper->updater.db_resources);
	free (ozookeeper->updater.db_online);

    }
    ozookeeper->updater.w_resources = new_w_resources;
    ozookeeper->updater.w_online = new_w_online_matrix;

    ozookeeper->updater.db_resources = new_db_resources;
    ozookeeper->updater.db_online = new_db_online_matrix;


    free (array);




    fprintf (stderr, "\nregistered computers:");
    for (iter = 0; iter < computers.count; iter++) {
	fprintf (stderr, "\n%s", computers.data[iter]);
    }
    fprintf (stderr, "\nnew computers:");
    for (iter = 0; iter < computers.count; iter++) {
	if (sort[iter] == -1) {
	    fprintf (stderr, "\n%s", computers.data[iter]);

	}
    }

//set watches on the new sources/get their information
    for (iter = 0; iter < computers.count; iter++) {
	if (sort[iter] == -1) {

	    sprintf (spath, "/%s/computers/%s/worker_nodes", octopus,
		     computers.data[iter]);
	    resources (ozookeeper, spath, start);

	    sprintf (spath, "/%s/computers/%s/db_nodes", octopus,
		     computers.data[iter]);
	    resources (ozookeeper, spath, start);

	}
    }
    free (sort);

}



void
w_computers (zhandle_t * zh, int type,
	     int state, const char *path, void *watcherCtx)
{

    ozookeeper_t *ozookeeper = (ozookeeper_t *) watcherCtx;


    if (type == ZOO_SESSION_EVENT
	&& (state == ZOO_EXPIRED_SESSION_STATE
	    || state == ZOO_AUTH_FAILED_STATE)) {
//do nothing global watcher will reinitialize things


    }
    else {

	computers (ozookeeper, 0);


    }

}





//completions

//i use this completion function simply to preserve the order of operations
//the computers function re"asks" for the computer list
void
c_computers (int rc, const struct String_vector *strings, const void *data)
{
    ozookeeper_t *ozookeeper = (ozookeeper_t *) data;

    computers (ozookeeper, 1);

    ozookeeper_update_go_online (ozookeeper, 1);
    ozookeeper_update_go_online (ozookeeper, 0);


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
    char octopus[1000];
    int result;
    char path[1000];


    oconfig_octopus (ozookeeper->config, octopus);
//initialize self
    sprintf (path, "/%s/computers", octopus);
    result =
	zoo_aget_children (ozookeeper->zh, path, 0, c_computers, ozookeeper);
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
global_watcher (zhandle_t * zzh, int type, int state,
		const char *path, void *context)
{


    fprintf (stderr, "Watcher %s state = %s", type2String (type),
	     state2String (state));
    if (path && strlen (path) > 0) {
	fprintf (stderr, " for path %s", path);
    }
    fprintf (stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
	if (state == ZOO_CONNECTED_STATE) {
	    fprintf (stderr,
		     "(Re)connected with session id: 0x%llx\n",
		     _LL_CAST_ (zoo_client_id (zzh))->client_id);
	}

	else {
	    if (state == ZOO_AUTH_FAILED_STATE) {
		fprintf (stderr, "Authentication failure. Retrying...\n");
		exit (1);
	    }
	    else if (state == ZOO_EXPIRED_SESSION_STATE) {
		fprintf (stderr, "Session expired. Retrying...\n");
		exit (1);
	    }
	}
    }
}
