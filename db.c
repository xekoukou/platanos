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


void db_msg(zmsg_t *msg, void * router){



}



void *
db_fn (void *arg)
{
    zctx_t *ctx = zctx_new ();

    db_t *db = (db_t *) arg;

    int rc;

//db infrastruct

    void *router = zsocket_new (ctx, ZMQ_ROUTER);
    void *dealer = zsocket_new (ctx, ZMQ_DEALER);

    char identity[17];  //14+2+1

    sprintf (identity, "%sdb", worker->id);
    zmq_setsockopt (dealer, ZMQ_IDENTITY, identity, strlen (identity));


zmq_pollitem_t pollitem[1]={{dealer,0,ZMQ_POLLIN}};

    fprintf (stderr, "\ndb with id:%s ready.", db->id);

while(1){
rc=zmq_poll(pollitem,1,-1){
assert(rc!=-1);

        if (pollitem[0].revents & ZMQ_POLLIN) {
            zmsg_t *msg=zmsg_recv(dealer);
           db_msg (msg,router);
        }


}}



}


void
db_init (db_t ** db, oconfig_t * config,
             char *comp_name, char *res_name)
{

    *db = malloc (sizeof (db_t));
    (*db)->res_name = malloc (strlen (res_name) + 1);
    strcpy ((*db)->res_name, res_name);
    (*db)->comp_name = malloc (strlen (comp_name) + 1);
    strcpy ((*db)->comp_name, comp_name);
    (*db)->id = malloc (strlen (comp_name) + strlen (res_name) + 1);
    sprintf ((*db)->id, "%s%s", comp_name, res_name);
    (*db)->config = config;

}

