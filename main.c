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


//Open the configuration file
    oconfig_t *config;
    oconfig_init (&config);


//bind to the apropriate location
    int rc;

    rc = zsocket_bind (w_pub, "tcp://127.0.0.1:49152");
    assert (rc == 49152);
    rc = zsocket_bind (w_router, "tcp://127.0.0.1:49153");
    assert (rc == 49153);
    rc = zsocket_bind (db_pub, "tcp://127.0.0.1:49154");
    assert (rc == 49154);
    rc = zsocket_bind (db_router, "tcp://127.0.0.1:49155");
    assert (rc == 49155);




    ozookeeper_t *ozookeeper;

    ozookeeper_init (&ozookeeper, config, w_pub, w_router, db_pub, db_router);

    workers_t *workers;
    workers_init (&workers, ctx, ozookeeper);

    ozookeeper_init_workers (ozookeeper, workers);

    ozookeeper_getconfig (ozookeeper);

    workers_monitor (workers);
    return 0;
}
