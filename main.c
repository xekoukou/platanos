#include<czmq.h>
#include<stdio.h>
#include<zookeeper/zookeeper.h>
#include"config.h"
#include"zookeeper.h"
#include"worker.h"




int
main ()
{

    zctx_t *ctx = zctx_new ();
    void *w_pub = zsocket_new (ctx, ZMQ_PUB);
    void *w_router = zsocket_new (ctx, ZMQ_ROUTER);
    void *db_pub = zsocket_new (ctx, ZMQ_PUB);
    void *db_router = zsocket_new (ctx, ZMQ_ROUTER);


//bind to the apropriate location

    int rc;

    zsocket_bind (w_pub, "ipc:///tmp/w_publisher");

    zsocket_bind (w_router, "ipc:///tmp/w_router");

    zsocket_bind (db_pub, "ipc:///tmp/db_publisher");

    zsocket_bind (db_router, "ipc:///tmp/db_router");



//Open the configuration file
    oconfig_t *config;
    oconfig_init (&config);

    ozookeeper_t *ozookeeper;

    ozookeeper_init (&ozookeeper, config, w_pub, w_router, db_pub, db_router);

    workers_t *workers;
    workers_init (&workers, ctx, ozookeeper);

    ozookeeper_init_workers (ozookeeper, workers);

    ozookeeper_getconfig (ozookeeper);

    workers_monitor (workers);
    return 0;
}
