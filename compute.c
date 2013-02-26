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


#include"compute.h"

void
compute_init (compute_t ** compute, khash_t (vertices) * hash,
              router_t * router, zlist_t * events, intervals_t * intervals,
              void *socket_nb, void *self_nb, void *socket_wb, void *self_wb,
              localdb_t * localdb, worker_t * worker, void *wake_nod)
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
    (*compute)->wake_nod = wake_nod;
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
    result = zoo_get (worker->zh, path, 0, (char *) buffer, &buffer_len, &stat);
    assert (result == ZOK);

    assert (buffer_len == sizeof (unsigned long));

    (*compute)->interval_size = buffer;



}
