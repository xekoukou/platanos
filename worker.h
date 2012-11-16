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
#include"hash/khash.h"
#include"localdb.h"
#include"vertex.h"
#include"router.h"
#include"zookeeper.h"

#ifndef OCTOPUS_WORKER_H_
#define OCTOPUS_WORKER_H_

//TODO I havent yet provided a destructor function
typedef struct
{
    zhandle_t *zh;
    oconfig_t *config;
    char *id;			//comp_name +res_name
    char *res_name;
    char *comp_name;
    int64_t next_time;
    int is_it_sleep;
} worker_t;

void worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config,
		  char *comp_name, char *res_name);


void worker_update_timeout (worker_t * worker, int new_next_time,
			    int is_it_sleep, void *wake_nod);


typedef struct
{
    router_t *router;
    zlist_t *events;
    intervals_t *intervals;
      khash_t (vertices) * hash;
    void *socket_nb;
    void *self_nb;
    void *socket_wb;
    void *self_wb;
    localdb_t *localdb;
    unsigned long counter;
    int interval;		//the interval in which the counter resides
    unsigned long interval_size;
    worker_t *worker;		//this is the object given to the thread, it also has
    //the zookeeper handle used to set the worker online or get
    //get the interval 
    void *wake_nod;

} compute_t;

void compute_init (compute_t ** compute, khash_t (vertices) * hash,
		   router_t * router, zlist_t * events,
		   intervals_t * intervals, void *socket_nb, void *self_nb,
		   void *socket_wb, void *self_wb, localdb_t * localdb,
		   worker_t * worker, void *wake_nod);



//arg is a const integer  
//great care not to change that integer
//it is used in the sub and dealer socket (subscription, identity)
//max 1000 workers per computer
//worker ids are from 0 till size-1
void *worker_fn (void *arg);

typedef struct
{
    pthread_t *pthread;
    char **id;			//has null at the end
    int size;
} workers_t;

struct ozookeeper_t;


void workers_init (workers_t ** workers, struct ozookeeper_t *ozookeeper);

void workers_monitor (workers_t * workers);


#endif
