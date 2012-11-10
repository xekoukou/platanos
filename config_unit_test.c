#include<stdio.h>
#include<string.h>
#include"config.h"


int
main ()
{



    oconfig_t *config;
    oconfig_init (&config);


    int port = oconfig_incr_port (config);

    printf ("\nThe new port:%d", port);


    oconfig_destroy (config);


    return 0;
}
