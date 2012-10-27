#include"localdb.h"
#include<czmq.h>

int
main ()
{

    localdb_t *localdb;

    char *id = "id456";

    localdb_init (&localdb, id);


    int result;

    result = localdb_read_counter (localdb);
    printf ("\n%d", result);

    localdb_incr_counter (localdb, 39);


    result = localdb_read_counter (localdb);
    printf ("\n%d", result);


    zclock_sleep (10000);

}
