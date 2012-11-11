#include"zookeeper.h"

#ifndef OCTOPUS_DB_H_
#define OCTOPUS_DB_H_


typedef struct
{
    pthread_t *pthread;
    char **id;			//has null at the end
    int size;
} dbs_t;


void dbs_init (dbs_t ** dbs, struct ozookeeper_t *ozookeeper);


#endif
