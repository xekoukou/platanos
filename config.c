#include<stdio.h>
#include<stdlib.h>
#include"config.h"
#include<string.h>

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
    memcpy (octopus, config->line[2], 1000);
}


void
oconfig_host (oconfig_t * config, char *host)
{
    memcpy (host, config->line[0], 1000);
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
    memcpy (comp_name, config->line[3], 1000);
}

void
oconfig_destroy (oconfig_t * config)
{

    free (config);

}
