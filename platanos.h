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


#ifndef OCTOPUS_PLATANOS_H_
#define OCTOPUS_PLATANOS_H_

#include"compute.h"
#include<zookeeper/zookeeper.h>


/*
struct platanos_poll_t{
zmq_pollitems_t *pollitems;
int size;
void *sockets;
int64_t next_time;
};


struct platanos_t{
platanos_poll_t *poll;
compute_t *compute;

};


*/

struct platanos_poll_t;
typedef struct platanos_poll_t platanos_poll_t;

struct platanos_t;
typedef struct platanos_t platanos_t;

struct compute_t;
typedef struct compute_t compute_t;


void platanos_poll_init (platanos_poll_t ** poll);

//returns the next time that the timeout of the poll needs to show
//other computations before the poll are also performed here, see PlatanosPriorityPoll
int64_t platanos_poll_before_poll (platanos_poll_t * poll);

void platanos_poll_pollitems (platanos_poll_t * poll,
                              zmq_pollitem_t ** pollitems, int *size);

void platanos_do (platanos_t * platanos);

platanos_poll_t *platanos_poll (platanos_t * platanos);

void platanos_init (platanos_t ** platanos, platanos_poll_t * poll,
                    compute_t * compute, char *id, zctx_t * ctx);


//here one can also add more registration options
//only for worker_nodes
void platanos_register (zhandle_t * zh, char *octopus, char *res_name,
                        char *bind_point);

//inside online
void platanos_bind_points (zhandle_t * zh, char *octopus, char *comp_name,
                           char *res_name, char ***bind_points, int *size);

void platanos_send (platanos_t * platanos, zmsg_t * msg);

/*
struct platanos_node_t {


}

*/
struct platanos_node_t;
typedef struct platanos_node_t platanos_node_t;


platanos_node_t *platanos_connect (platanos_t * platanos, zmsg_t * msg);
platanos_node_t *platanos_bind (platanos_t * platanos, zmsg_t * msg);

void platanos_connect_to_db (platanos_t * platanos, char *bind_location);

void platanos_node_destroy (platanos_node_t ** platanos_node);

platanos_node_t *platanos_node_dup (platanos_node_t * platanos_node);


void platanos_db_do (zmsg_t * msg, void *out);
#endif
