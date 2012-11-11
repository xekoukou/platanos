#ifndef _OCTOPUS_ZOOKEEPER_H_
#define _OCTOPUS_ZOOKEEPER_H_

#include<zookeeper/zookeeper.h>
#include"config.h"
#include"worker.h"
#include"db.h"

#define _LL_CAST_ (long long)

typedef struct
{
    unsigned int id;		//used to id updates
    char *key;			//computer + res_name that is also the id of all communication to the worker 
//contains null

//these are used to find the changes that happened to the children
    struct String_vector computers;
    struct String_vector *w_resources;
    int **w_online;		//this is used tocheck whether a previous watch event has set a resource
//offline (used by w_st_piece ,w_n_pieces)

    struct String_vector *db_resources;
    int **db_online;

} oz_updater_t;

struct ozookeeper_t
{
    zhandle_t *zh;
    oconfig_t *config;
    void *w_pub;
    void *w_router;
    void *db_pub;
    void *db_router;
    oz_updater_t updater;
    workers_t *workers;
};

typedef struct ozookeeper_t ozookeeper_t;



//initialize the ozookeeper object
void
ozookeeper_init (ozookeeper_t ** ozookeeper, oconfig_t * config,
		 void *w_pub, void *w_router, void *db_pub, void *db_router);


int ozookeeper_not_corrupt (ozookeeper_t ** ozookeep);

void ozookeeper_init_workers (ozookeeper_t * ozookeeper, workers_t * workers);

void ozookeeper_init_dbs (ozookeeper_t * ozookeeper, dbs_t * dbs);

void ozookeeper_getconfig (ozookeeper_t * ozookeeper);

void ozookeeper_set_zhandle (ozookeeper_t * ozookeeper, zhandle_t * zh);

void ozookeeper_zhandle (ozookeeper_t * ozookeeper, zhandle_t ** zh);

void ozookeeper_destroy (ozookeeper_t * ozookeeper);

void global_watcher (zhandle_t * zzh, int type, int state, const char *path,
		     void *context);

// doesnt allocate memory
void oz_updater_init (oz_updater_t * updater);

void oz_updater_destroy (oz_updater_t * updater);

void oz_updater_key (oz_updater_t * updater, char *key);

void oz_updater_free_key (oz_updater_t * updater);

void
oz_updater_search (oz_updater_t * updater, int db, char *comp_name,
		   char *res_name, int *m, int *n);
#endif
