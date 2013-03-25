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





//leveldb
#include"dbo.h"


void
dbo_init (dbo_t ** dbo)
{
    *dbo = malloc (sizeof (dbo_t));


    leveldb_options_t *options = leveldb_options_create ();
//manually initialize it
    leveldb_options_set_create_if_missing (options, 0);

    (*dbo)->options = options;


    leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
    (*dbo)->readoptions = readoptions;
    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    (*dbo)->writeoptions = writeoptions;



    leveldb_writeoptions_set_sync (writeoptions, 1);

}

void
dbo_open (dbo_t * dbo, char *location)
{

    dbo->location = malloc (strlen (location) + 1);
    strcpy (dbo->location, location);

    char *errptr = NULL;

    dbo->db = leveldb_open (dbo->options, location, &errptr);
    if (errptr) {
        printf ("\n%s", errptr);
        exit (1);
    }


}

//needs a few seconds to close
//put a sleep after it
void
dbo_close (dbo_t * dbo)
{
    leveldb_close (dbo->db);
}

void
dbo_destroy (dbo_t ** dbo)
{
    dbo_close (*dbo);

    free ((*dbo)->location);
    free (*dbo);
    *dbo = NULL;
//TODO free options etc.

}
