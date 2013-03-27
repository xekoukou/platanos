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

#ifndef OCTOPUS_COMPUTE_H_
#define OCTOPUS_COMPUTE_H_

#include "localdb.h"
#include "worker.h"

struct worker_t;
typedef struct worker_t worker_t;

struct router_t;
typedef struct router_t router_t;

struct compute_t
{
    router_t *router;
    router_t *db_router;
    zlist_t *events;
    intervals_t *intervals;
      khash_t (vertices) * hash;
    localdb_t *localdb;
    unsigned long counter;
    int interval;               //the interval in which the counter resides
    unsigned long interval_size;
    worker_t *worker;           //this is the object given to the thread, it also has
    //the zookeeper handle used to set the worker online or get
    //get the interval 

};

typedef struct compute_t compute_t;


void compute_init (compute_t ** compute, khash_t (vertices) * hash,
                   router_t * router, router_t * db_router, zlist_t * events,
                   intervals_t * intervals,
                   localdb_t * localdb, worker_t * worker);



#endif
