//leveldb
#include<leveldb/c.h>
#include<string.h>
#include"localdb.h"
#include<stdlib.h>
#include<stdio.h>



//id is the address/id of the thread/node
int
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
int
localdb_close (localdb_t * localdb)
{
    leveldb_close (localdb->db);

}

int
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
	printf ("\n%u", errptr);
	exit (1);

    }

}

int
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
	memcpy (&counter, result, sizeof (int));
	return interval;
    }
    else {
	printf ("\n%u", errptr);
	exit (1);
    }
}
