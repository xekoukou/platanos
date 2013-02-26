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

#include"workers.h"

void
workers_init (workers_t ** workers, ozookeeper_t * ozookeeper)
{

    int result;
    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);

    sprintf (path, "/%s/computers/%s/worker_nodes", octopus, comp_name);
    struct String_vector worker_children;
    result = zoo_get_children (ozookeeper->zh, path, 0, &worker_children);


    if (ZOK == result) {

//mallocing
        *workers = malloc (sizeof (workers_t));
        (*workers)->size = worker_children.count;
        (*workers)->id = malloc (sizeof (char *) * (worker_children.count));
        (*workers)->pthread =
            malloc (sizeof (pthread_t) * worker_children.count);

//create the threads

        int iter;
        if (worker_children.count < 1000) {
            worker_t *worker;
            for (iter = 0; iter < worker_children.count; iter++) {
                (*workers)->id[iter] =
                    malloc (strlen (worker_children.data[iter]) + 1 +
                            strlen (comp_name));

                sprintf ((*workers)->id[iter], "%s%s", comp_name,
                         worker_children.data[iter]);

                worker_init (&worker, ozookeeper->zh, ozookeeper->config,
                             comp_name, worker_children.data[iter]
                    );

                pthread_create (&((*workers)->pthread[iter]), NULL, worker_fn,
                                worker);
            }
        }
        else {
            fprintf (stderr, "\n More workers than allowed.. error exiting");
            exit (1);
        }
        if (ZOK != result && ZNONODE != result) {
            fprintf (stderr, "\n Couldnt get the children.. error exiting");
            exit (1);
        }


        deallocate_String_vector (&worker_children);
    }




}
