#ifndef _OCTOPUS_LOCALDB_H_
#define _OCTOPUS_LOCALDB_H_


//leveldb
#include<leveldb/c.h>


typedef struct
{
    leveldb_t *db;
    leveldb_options_t *options;
    leveldb_readoptions_t *readoptions;
    leveldb_writeoptions_t *writeoptions;

} localdb_t;


//id is the address/id of the thread/node
int localdb_init (localdb_t ** localdb, char *id);

//sleep a few seconds after
int localdb_close (localdb_t * localdb);

int localdb_incr_counter (localdb_t * localdb, unsigned long counter);

unsigned long localdb_get_counter (localdb_t * localdb);

int localdb_set_interval (localdb_t * localdb, int interval);

int localdb_get_interval (localdb_t * localdb);


#endif
