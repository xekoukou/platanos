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

#include"db.h"


void
db__balance (db_balance_t * balance)
{

    zmsg_t *msg = zmsg_recv (balance->self_bl);
    if (!msg) {
        exit (1);
    }

    db_balance_new_msg (balance, msg);

}


void
db__balance_lazy_pirate (db_balance_t * balance)
{

    db_balance_lazy_pirate (balance);

}



void
db_add_self (db_update_t * update, zmsg_t * msg)
{
    node_t *self;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point[50];
    char bind_point_bl[50];
    char db_location[1000];



    zframe_t *frame = zmsg_first (msg);

    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_bl, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (db_location, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    fprintf (stderr,
             "\ndb:%s:add_self:\nkey:%s\nn_pieces:%d\nst_piece:%lu\nlocation:%s",
             key, key, n_pieces, st_piece, db_location);

    db_node_init (&self, key, n_pieces, st_piece, bind_point);

//update router
    update->db_router->self = self;

//open the database
    dbo_open (update->balance->dbo, db_location);

//the rest of the router update will happen when the node goes online

//bind sockets
    int rc;
    rc = zsocket_bind (update->in, "%s", bind_point);
    assert (rc != -1);
//dbs will never talk to themselves
//out doesnt need to connect to in

    rc = zsocket_bind (update->balance->self_bl, "%s", bind_point_bl);
    assert (rc != -1);
    rc = zsocket_connect (update->balance->router_bl, "%s", bind_point_bl);
    assert (rc == 0);

    fprintf (stderr, "\ndb:%s:add_self: received its configuration",
             update->balance->self_key);


}


db_add_dead_node (update_t * update, zmsg_t * msg)
{
    node_t *node;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point_bl[50];

    zframe_t *frame = zmsg_first (msg);
    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_bl, zframe_data (frame), zframe_size (frame));


    zmsg_destroy (&msg);

//TODO disconnect

    fprintf (stderr,
             "\n%s:db_add_dead_node:\nkey:%s\nn_pieces:%d\nst_piece:%lu",
             update->balance->self_key, key, n_pieces, st_piece);


    node = nodes_search (update->db_router->nodes, key);

    if (node != NULL) {

        node_set_alive (node, 0);

    }
    else {
        db_node_init (&node, key, n_pieces, st_piece, bind_point_bl);

//set the node to alive
        node_set_alive (node, 0);

//update router object
        assert (1 == router_add (update->db_router, node));
    }

//TODO on_give on_receive?
}



void
db_delete_node (db_update_t * update, zmsg_t * msg)
{
    char key[100];
    zframe_t *frame = zmsg_first (msg);
    memcpy (key, zframe_data (frame), zframe_size (frame));

    zmsg_destroy (&msg);

    node_t *node = nodes_search (update->router->nodes, key);
    assert (node != NULL);

//TODO on_give, on_receive?

}

void
db_add_node (db_update_t * update, zmsg_t * msg)
{

    node_t *node;
    char key[100];
    int n_pieces;
    unsigned long st_piece;
    char bind_point_bl[50];

    zframe_t *frame = zmsg_first (msg);
    //first frame is used only be the worker
    frame = zmsg_next (msg);
    memcpy (key, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&n_pieces, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (&st_piece, zframe_data (frame), zframe_size (frame));
    frame = zmsg_next (msg);
    memcpy (bind_point_bl, zframe_data (frame), zframe_size (frame));

//the bind_point_db is not needed
    zmsg_destroy (&msg);

    int rc;
    rc = zsocket_connect (update->balance->router_bl, "%s", bind_point_bl);
    assert (rc == 0);




    fprintf (stderr,
             "\n%s:add_node: added node with\nstart:%d\nkey:%s\nn_pieces:%d\nst_piece:%lu",
             update->balance->self_key, start, key, n_pieces, st_piece);

    db_node_init (&node, key, n_pieces, st_piece, bind_point_bl);
    node_set_alive (node, 1);

//update router object
//this should always happen after the prev step
    assert (1 == router_add (update->router, node));


    intervals_t *cintervals = router_current_intervals (update->router, node);

    db_balance_init_gives (cintervals);

}

void
db_update_st_piece (db_update_t * update, zmsg_t * msg)
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
    prev_node = nodes_search (update->db_router->nodes, key);
    assert (prev_node != NULL);
    node = node_dup (prev_node);
    node->st_piece = st_piece;


//update router object

    router_delete (update->db_router, prev_node);
    router_add (update->db_router, node);

    db_balance_init_gives (cintervals);

}

void
db_update_n_pieces (db_update_t * update, zmsg_t * msg)
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
    prev_node = nodes_search (update->db_router->nodes, key);
    assert (prev_node != NULL);
    node_dup (prev_node);
    node->n_pieces = n_pieces;

//update router object

    router_delete (update->db_router, prev_node);
    router_add (update->db_router, node);


    db_balance_init_gives (cintervals);

}

void
db_go_online (db_t * db)
{

    char path[1000];
    char octopus[1000];

    oconfig_octopus (db->config, octopus);

    sprintf (path, "/%s/computers/%s/db_nodes/%s/online", octopus,
             db->comp_name, db->res_name);

    int result = zoo_create (db->zh, path, NULL,
                             -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL,
                             0);
    assert (result == ZOK);

    fprintf (stderr, "\ndb:%s:go_online: has gone online", db->id);

}



void
db_update (db_update_t * update, void *sub)
{

//check if it is a new update or an old one
    zmsg_t *msg = zmsg_recv (sub);
    if (!msg) {
        exit (1);
    }

    fprintf (stderr, "\ndb:%s:update:I have received a sub msg",
             update->balance->self_key);
    zframe_t *fr = zmsg_pop (msg);
    zframe_destroy (&fr);
    zframe_t *id = zmsg_pop (msg);
    if (memcmp (zframe_data (id), &(update->id), sizeof (unsigned int)) == 0) {
//lazy pirate reconfirm update
        zframe_send (&id, update->dealer, 0);
        zframe_destroy (&id);
        zmsg_destroy (&msg);
        fprintf (stderr,
                 "\ndb:%s:update:It was a previous update, resending confirmation",
                 update->balance->self_key);

    }
    else {
        fr = zmsg_pop (msg);
        zframe_destroy (&fr);

        zframe_t *frame = zmsg_pop (msg);
        if (memcmp (zframe_data (frame), "add_self", zframe_size (frame)) == 0) {
            db_add_self (update, msg);
        }
        else {
            if (memcmp
                (zframe_data (frame), "add_dead_node",
                 zframe_size (frame)) == 0) {
                db_add_dead_node (update, msg);
            }
            else {
                if (memcmp
                    (zframe_data (frame), "delete_node",
                     zframe_size (frame)) == 0) {
                    db_delete_node (update, msg);
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
                                    db_go_online (update->db);
                                }




                            }
                        }
                    }
                }
            }
        }
        zframe_destroy (&frame);


        zframe_send (&id, update->dealer, 0);
        fprintf (stderr,
                 "\ndb:%s:update:I have sent confirmation to sub msg",
                 update->balance->self_key);

    }
}





void *
db_fn (void *arg)
{
    zctx_t *ctx = zctx_new ();

    db_t *db = (db_t *) arg;

    int rc;

//update infrastructure
    void *sub = zsocket_new (ctx, ZMQ_SUB);
    void *dealer = zsocket_new (ctx, ZMQ_DEALER);

    char identity[17];          //14+2+1

    sprintf (identity, "%sdb", db->id);
    zmq_setsockopt (dealer, ZMQ_IDENTITY, identity, strlen (identity));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, identity, strlen (identity));
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "db", strlen ("db") + 1);
    zmq_setsockopt (sub, ZMQ_SUBSCRIBE, "all", strlen ("all") + 1);


    rc = zsocket_connect (sub, "tcp://127.0.0.1:49152");
    assert (rc == 0);

    rc = zsocket_connect (dealer, "tcp://127.0.0.1:49153");
    assert (rc == 0);


//db infrastruct

    void *out = zsocket_new (ctx, ZMQ_ROUTER);
    void *in = zsocket_new (ctx, ZMQ_DEALER);


    sprintf (identity, "%sdb", db->id);
    zmq_setsockopt (in, ZMQ_IDENTITY, identity, strlen (identity));


//balance infrastructure

    void *router_bl = zsocket_new (ctx, ZMQ_ROUTER);
    void *self_bl = zsocket_new (ctx, ZMQ_DEALER);
    zmq_setsockopt (self_bl, ZMQ_IDENTITY, db->id, strlen (db->id));


    dbo_t *dbo;

    dbo_init (&dbo);

//router object
//only used for balancing , not for routing msgs
//databases never talk to themselves
    router_t *db_router;

    router_init (&db_router, 1);


//balance object
    db_balance_t *balance;

    db_balance_init (&balance, dbo, router_bl, self_bl, db->id);

//update object
//used to update things, like the router object
    db_update_t *update;

    db_update_init (&update, dealer, db_router, balance, db, in, out);



    zmq_pollitem_t pollitem[2] =
        { {in, 0, ZMQ_POLLIN}, {sub, 0, ZMQ_POLLIN}, {self_bl, 0, ZMQ_POLLIN} };

    fprintf (stderr, "\ndb:%s ready.", db->id);


    while (1) {
        int64_t timeout = db_timeout (balance);

        if(timeout == 0){
    db_process_timer_events (db, balance);
}
        rc = zmq_poll (pollitem, 1, db_timeout (balance));
        assert (rc != -1);
    
        if (pollitem[0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (in);
            platanos_db_do (msg, out);
        }
        if (pollitem[1].revents & ZMQ_POLLIN) {
            db_update (update, sub);
        }
        if (pollitem[2].revents & ZMQ_POLLIN) {
            db_balance (balance, self_bl);
        }


    }
}

int64_t
db_timeout (db_balance_t * balance)
{
    int64_t time = zclock_time ();

    int64_t timeout = -1;
    if (last > 0) {
        timeout = balance->next_time - time;
        if (timeout < 0) {
            timeout = 0;
        }

    }
}


void
db_process_timer_events (db_t * db, db_balance_t * balance)
{
    int64_t time = zclock_time ();

    if (balance->next_time < time) {
        db_balance_lazy_pirate (balance);
    }
}



void
db_init (db_t ** db, zhandle_t * zh, oconfig_t * config, char *comp_name,
         char *res_name)
{

    *db = malloc (sizeof (db_t));
    (*db)->zh = zh;
    (*db)->res_name = malloc (strlen (res_name) + 1);
    strcpy ((*db)->res_name, res_name);
    (*db)->comp_name = malloc (strlen (comp_name) + 1);
    strcpy ((*db)->comp_name, comp_name);
    (*db)->id = malloc (strlen (comp_name) + strlen (res_name) + 1);
    sprintf ((*db)->id, "%s%s", comp_name, res_name);
    (*db)->config = config;

}
