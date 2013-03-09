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



#include"dbs.h"


void
dbs_init (dbs_t ** dbs, ozookeeper_t * ozookeeper)
{

    int result;
    char path[1000];
    char octopus[1000];
    char comp_name[1000];

    oconfig_octopus (ozookeeper->config, octopus);
    oconfig_comp_name (ozookeeper->config, comp_name);

    sprintf (path, "/%s/computers/%s/db_nodes", octopus, comp_name);
    struct String_vector db_children;
    result = zoo_get_children (ozookeeper->zh, path, 0, &db_children);


    if (ZOK == result) {

//mallocing
        *dbs = malloc (sizeof (dbs_t));
        (*dbs)->size = db_children.count;
        (*dbs)->id = malloc (sizeof (char *) * (db_children.count));
        (*dbs)->pthread = malloc (sizeof (pthread_t) * db_children.count);

//create the threads

        int iter;
        if (db_children.count < 1000) {
            db_t *db;
            for (iter = 0; iter < db_children.count; iter++) {
                (*dbs)->id[iter] =
                    malloc (strlen (db_children.data[iter]) + 1 +
                            strlen (comp_name));

                sprintf ((*dbs)->id[iter], "%s%s", comp_name,
                         db_children.data[iter]);

                db_init (&db, ozookeeper->zh, ozookeeper->config,
                         comp_name, db_children.data[iter]
                    );

                pthread_create (&((*dbs)->pthread[iter]), NULL, db_fn, db);
            }
        }
        else {
            fprintf (stderr, "\n More dbs than allowed.. error exiting");
            exit (1);
        }
        if (ZOK != result && ZNONODE != result) {
            fprintf (stderr, "\n Couldnt get the children.. error exiting");
            exit (1);
        }


        deallocate_String_vector (&db_children);
    }
}
