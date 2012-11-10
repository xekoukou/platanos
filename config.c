#include<stdio.h>
#include<stdlib.h>
#include"config.h"
#include<string.h>
#include<assert.h>

// the configuration file is always at the location that the program starts

void
oconfig_init (oconfig_t ** config)
{


    *config = (oconfig_t *) malloc (sizeof (oconfig_t));

//Open the configuration file
    FILE *fconfig = fopen ("./config", "r");
    if (fconfig == NULL) {
	printf
	    ("\nconfig doesnt exist. Are you in the correnct directory? A config file must be created manually for every computer that is part of the octopus.. exiting");
	exit (1);
    }

    int line_position = 0;
    while ((fgets ((*config)->line[line_position], 1000, fconfig))
	   && (line_position < 30)) {
	(*config)->line[line_position][strlen ((*config)->line[line_position])
				       - 1] = '\0';



	line_position++;
	(*config)->n_lines = line_position;
    }
    fclose (fconfig);

}


void
oconfig_octopus (oconfig_t * config, char *octopus)
{
    strcpy (octopus, config->line[2]);
}


void
oconfig_host (oconfig_t * config, char *host)
{
    strcpy (host, config->line[0]);
}

void
oconfig_recv_timeout (oconfig_t * config, int *timeout)
{
    *timeout = atoi (config->line[1]);
}

//computer name
void
oconfig_comp_name (oconfig_t * config, char *comp_name)
{
    strcpy (comp_name, config->line[3]);
}

void
oconfig_destroy (oconfig_t * config)
{

    free (config);

}

int
oconfig_port (oconfig_t * config)
{
    char temp[100];
    strcpy (temp, config->line[4]);
    return atoi (temp);

}

int
oconfig_incr_port (oconfig_t * config)
{

//Open the configuration file
    FILE *fconfig = fopen ("./config", "r+");
    if (fconfig == NULL) {
	printf
	    ("\nconfig doesnt exist. Are you in the correnct directory? A config file must be created manually for every computer that is part of the octopus.. exiting");
	exit (1);
    }

    char temp[1000];
    int iter;
    for (iter = 0; iter < 4; iter++) {
	fgets (temp, 1000, fconfig);

    }

    int port = oconfig_port (config);
    port++;

    sprintf (temp, "%d", port);
   strcpy (config->line[4],temp);

    int rc = fputs (temp, fconfig);
    assert (rc > 0);
    fclose (fconfig);

    return port;




}
