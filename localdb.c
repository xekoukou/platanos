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
#include<leveldb/c.h>
#include<string.h>
#include"localdb.h"
#include<stdlib.h>
#include<stdio.h>



//id is the address/id of the thread/node
void
localdb_init (localdb_t ** localdb, char *id)
{
    *localdb = (localdb_t *) malloc (sizeof (localdb_t));

    char *errptr = NULL;

    leveldb_options_t *options = leveldb_options_create ();
    leveldb_options_set_create_if_missing (options, 1);

    (*localdb)->options = options;


    leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
    (*localdb)->readoptions = readoptions;
    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    (*localdb)->writeoptions = writeoptions;



    leveldb_writeoptions_set_sync (writeoptions, 1);

    char path[1000];

    sprintf (path, "/mnt/localdb/%s", id);
    (*localdb)->db = leveldb_open (options, path, &errptr);
    if (errptr) {
	printf ("\n%s", errptr);
	exit (1);
    }
}

//needs a few seconds to close
//put a sleep after it
void
localdb_close (localdb_t * localdb)
{
    leveldb_close (localdb->db);

}

void
localdb_incr_counter (localdb_t * localdb, unsigned long counter)
{

    char *errptr = NULL;

    leveldb_put (localdb->db,
		 localdb->writeoptions,
		 "\0", 1,
		 (const char *) &counter, sizeof (unsigned long), &errptr);

    if (errptr) {
	printf ("\n%s", errptr);
	exit (1);
    }


}

//if it doesnt exist ,return -1
//this means that things work fine but its just the beginning

unsigned long
localdb_get_counter (localdb_t * localdb)
{

    char *errptr = NULL;
    unsigned long counter;
    size_t vallen;

    char *result = leveldb_get (localdb->db,
				localdb->readoptions,
				"\0", 1,
				&vallen,
				&errptr);

    if (errptr) {
	printf ("\n%s", errptr);
	exit (1);
    }
    if (result == NULL) {
	return -1;
    }
    if (vallen == sizeof (unsigned long)) {
	memcpy (&counter, result, sizeof (unsigned long));
	return counter;
    }
    else {
	printf ("\n%s", errptr);
	exit (1);

    }

}

void
localdb_set_interval (localdb_t * localdb, int interval)
{

    char *errptr = NULL;

    leveldb_put (localdb->db,
		 localdb->writeoptions,
		 "\1", 1, (const char *) &interval, sizeof (int), &errptr);

    if (errptr) {
	printf ("\n%s", errptr);
	exit (1);
    }


}


int
localdb_get_interval (localdb_t * localdb)
{

    char *errptr = NULL;
    int interval;
    size_t vallen;

    char *result = leveldb_get (localdb->db,
				localdb->readoptions,
				"\1", 1,
				&vallen,
				&errptr);

    if (errptr) {
	printf ("\n%s", errptr);
	exit (1);
    }
    if (result == NULL) {
	return -1;
    }
    if (vallen == sizeof (int)) {
	memcpy (&interval, result, sizeof (int));
	return interval;
    }
    else {
	printf ("\n%s", errptr);
	exit (1);
    }
}
