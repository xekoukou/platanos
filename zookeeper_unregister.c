#include<zookeeper/zookeeper.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include"config.h"
#include<unistd.h>

#define _LL_CAST_ (long long)


static const char *
state2String (int state)
{
    if (state == 0)
	return "CLOSED_STATE";
    if (state == ZOO_CONNECTING_STATE)
	return "CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)
	return "ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)
	return "CONNECTED_STATE";
    if (state == ZOO_EXPIRED_SESSION_STATE)
	return "EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)
	return "AUTH_FAILED_STATE";

    return "INVALID_STATE";
}

static const char *
type2String (int state)
{
    if (state == ZOO_CREATED_EVENT)
	return "CREATED_EVENT";
    if (state == ZOO_DELETED_EVENT)
	return "DELETED_EVENT";
    if (state == ZOO_CHANGED_EVENT)
	return "CHANGED_EVENT";
    if (state == ZOO_CHILD_EVENT)
	return "CHILD_EVENT";
    if (state == ZOO_SESSION_EVENT)
	return "SESSION_EVENT";
    if (state == ZOO_NOTWATCHING_EVENT)
	return "NOTWATCHING_EVENT";

    return "UNKNOWN_EVENT_TYPE";
}

void
global_watcher (zhandle_t * zzh, int type, int state, const char *path,
		void *context)
{


    fprintf (stderr, "Watcher %s state = %s", type2String (type),
	     state2String (state));
    if (path && strlen (path) > 0) {
	fprintf (stderr, " for path %s", path);
    }
    fprintf (stderr, "\n");

    if (type == ZOO_SESSION_EVENT) {
	if (state == ZOO_CONNECTED_STATE) {
	    fprintf (stderr, "Reconnected with session id: 0x%llx\n",
		     _LL_CAST_ ((zoo_client_id (zzh))->client_id));
	}
    }
    else {
	if (state == ZOO_AUTH_FAILED_STATE) {
	    fprintf (stderr, "Authentication failure. Shutting Down...\n");
	    zookeeper_close (zzh);

	}
	else if (state == ZOO_EXPIRED_SESSION_STATE) {
	    fprintf (stderr, "Session expired. Shutting Down...\n");
	    zookeeper_close (zzh);

	}
    }
}



//strings count data

int
main ()
{

    printf ("\nI am going to unregister the resource that you describe");



    int result;


    char host[1000];
    char path[1000];

    printf ("\n What is the location of the zookeeper?");
    scanf ("%s", host);

    zhandle_t *zh = zookeeper_init (host, global_watcher, 3000, 0, 0, 0);

    sleep (1);

    int db;
    printf ("\nIs it a db_node(1) or a worker_node(0)?");
    scanf ("%d", &db);

    if (db != 1 && db != 0) {
	printf ("\n Error");
	exit (0);
    }

    char root[1000];

    if (db) {
	strcpy (root, "db_nodes");
    }
    else {
	strcpy (root, "worker_nodes");
    }

    char octopus[1000];
    printf ("\nWhat is the name of the octopus?");
    scanf ("%s", octopus);

    char comp_name[1000];
    printf ("\nWhat is the name of the computer?");
    scanf ("%s", comp_name);


    sprintf (path, "/%s/computers/%s/worker_nodes", octopus, comp_name);
    struct String_vector worker_children;
    result = zoo_get_children (zh, path, 0, &worker_children);
    if (ZOK != result) {
	printf ("\n Couldnt get the children.. exiting");
	return 1;
    }

    sprintf (path, "/%s/computers/%s/db_nodes", octopus, comp_name);
    struct String_vector db_children;
    result = zoo_get_children (zh, path, 0, &db_children);
    if (ZOK != result) {
	printf ("\n Couldnt get the children.. exiting");
	return 1;
    }


    char res_name[1000];
    printf ("\nWhat is the name of the resource?");
    scanf ("%s", res_name);


    sprintf (path, "/%s/computers/%s/%s/%s/n_pieces", octopus, comp_name,
	     root, res_name);
    result = zoo_delete (zh, path, -1);
    if (ZOK != result && ZOK != ZNONODE) {
	printf ("\n Error..cant remove n_pieces, exiting");
	return 1;
    }

    sprintf (path, "/%s/computers/%s/%s/%s/st_piece", octopus, comp_name,
	     root, res_name);
    result = zoo_delete (zh, path, -1);
    if (ZOK != result && ZOK != ZNONODE) {
	printf ("\n Error..,cant remove st_pieces, exiting");
	return 1;
    }


    sprintf (path, "/%s/computers/%s/%s/%s/bind_point", octopus, comp_name,
	     root, res_name);
    result = zoo_delete (zh, path, -1);
    if (ZOK != result && ZOK != ZNONODE) {
	printf ("\n Error.. cant remove bind+point, exiting");
	return 1;
    }

    if (db) {
	sprintf (path, "/%s/computers/%s/%s/%s/db_point", octopus, comp_name,
		 root, res_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}


    }

    sprintf (path, "/%s/computers/%s/%s/%s", octopus, comp_name, root,
	     res_name);
    result = zoo_delete (zh, path, -1);
    if (ZOK != result && ZOK != ZNONODE) {
	printf ("\n Error.. cant remove res_name node, exiting");
	return 1;
    }


//no more than 1 registration/unregistration should happen concurrently
    if ((worker_children.count + db_children.count) == 1) {
	printf
	    ("\nThere are no more resources registered in this computer, deleting the computer node as well");

	sprintf (path, "/%s/computers/%s/resources/max_memory", octopus,
		 comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}

	sprintf (path, "/%s/computers/%s/resources/free_memory", octopus,
		 comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}

	sprintf (path, "/%s/computers/%s/resources", octopus, comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}



	sprintf (path, "/%s/computers/%s/worker_nodes", octopus, comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}

	sprintf (path, "/%s/computers/%s/db_nodes", octopus, comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}

	sprintf (path, "/%s/computers/%s", octopus, comp_name);
	result = zoo_delete (zh, path, -1);
	if (ZOK != result && ZOK != ZNONODE) {
	    printf ("\n Error.. exiting");
	    return 1;
	}



    }


    result = remove ("./config");

    if (result) {
	printf ("\nCouldnt delete config, delete it manually");
    }


}
