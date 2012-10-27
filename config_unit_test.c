#include<stdio.h>
#include<string.h>
#include"config.h"


int
main ()
{

    int iter;
    int passed = 1;

    FILE *fconfig = fopen ("./config", "w");

    fprintf (fconfig, "Platonas_octopus\n");
    fprintf (fconfig, "db_node\n");
    fprintf (fconfig, "127.0.0.1:4565,197.45.23.21:89\n");
    fprintf (fconfig, "3000\n");
    fprintf (fconfig, "computer_name\n");
    fprintf (fconfig, "core1\n");
    fprintf (fconfig, "125\n");
    fprintf (fconfig, "12.34.56.78:2456\n");


    fclose (fconfig);

    oconfig_t *config;
    oconfig_init (&config);

    if (oconfig_db_node (config) != 1) {
	passed = 0;
    }


    char temp[1000];
    oconfig_host (config, temp);
    if (strcmp ("127.0.0.1:4565,197.45.23.21:89", temp)) {
	passed = 0;
	printf ("\n%s", temp);
    }

    oconfig_octopus (config, temp);
    if (strcmp ("Platonas_octopus", temp)) {
	passed = 0;
	printf ("\n%s", temp);
    }


    oconfig_bind_point (config, temp);
    if (strcmp ("12.34.56.78:2456", temp)) {
	passed = 0;
	printf ("\n%s", temp);
    }

    oconfig_comp_name (config, temp);
    if (strcmp ("computer_name", temp)) {
	passed = 0;
	printf ("\n%s", temp);
    }


    int int_temp;
    oconfig_recv_timeout (config, &int_temp);
    if (int_temp != 3000) {
	passed = 0;
	printf ("\n%d", int_temp);
    }

    oconfig_n_pieces (config, &int_temp);
    if (int_temp != 125) {
	passed = 0;
	printf ("\n%d", int_temp);
    }

    oconfig_res_name (config, temp);
    if (strcmp ("core1", temp)) {
	passed = 0;
	printf ("\n%s", temp);
    }


    if (config->n_lines != 8) {
	passed = 0;
	printf ("\n%d", config->n_lines);
    }


    oconfig_destroy (config);

    if (passed) {
	printf ("\n 1 config test passed");
    }
    else {
	printf ("\n 0 config test FAILED");

    }

    return 0;
}
