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
    void *pub = zsocket_new (ctx, ZMQ_PUB);
    void *router = zsocket_new (ctx, ZMQ_ROUTER);

//bind to the apropriate location

    int rc;

    rc = zsocket_bind (pub, "tcp://127.0.0.1:40000");
    assert (rc == 40000);

    rc = zsocket_bind (router, "tcp://127.0.0.1:40001");
    assert (rc == 40001);



//Open the configuration file
    oconfig_t *config;
    oconfig_init (&config);

    ozookeeper_t *ozookeeper;
    global_watcherctx_t *watcherctx;

    global_watcherctx_init (&watcherctx, config);
    ozookeeper_init (&ozookeeper, config, watcherctx, pub, router);

    workers_t *workers;
    workers_init (&workers, ctx, ozookeeper);

    ozookeeper_init_workers (ozookeeper, workers);

    ozookeeper_getconfig (ozookeeper);

    workers_monitor (workers);
    return 0;
}
