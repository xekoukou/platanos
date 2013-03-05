/*
    Copyright contributors as noted in the AUTHORS file.
                
    This file is part of PLATANOS.

    PLATANOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU Affero General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
            
    PLATANOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.
        
    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/




#include<zookeeper/zookeeper.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<proc/sysinfo.h>


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

int
main ()
{
    printf ("\nInitializing some global varriables of Octopus");

    char octopus[1000];
    printf ("\n What is the name of this Octopus?");
    scanf ("%s", octopus);

    printf
        ("\nEach vertex decides the quorum context it will use and is responsibe to fix things in case a replica doesnt respond. Each vertex needs to know the global number of replicas that exist though. \nWhat is the replication factor of the database?(number)");
    int replication;
    scanf ("%d", &replication);

    unsigned long interval_size;
    printf
        ("\nWhat is the size of the interval that is given to each worker node so as to create new vertices independently?(number)");

    scanf ("%u", &interval_size);

    printf
        ("\nPlease, provide the connecting points (ip:port,ip:port) to the zookeeper ensemple");
    char host[1000];
    scanf ("%s", host);
    zhandle_t *zh = zookeeper_init (host, global_watcher, 3000, 0, 0, 0);



    int result;
    char path[1000];

    sprintf (path, "/%s", octopus);
    result = zoo_create (zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        if (result == ZNODEEXISTS) {
            printf ("\nThere is another octopus with the same name");
            return 0;
        }
        else {
            printf ("\nError");
            return 1;
        }
    }

    sprintf (path, "/%s/computers", octopus);
    result = zoo_create (zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }


    sprintf (path, "/%s/global_properties", octopus);
    result = zoo_create (zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }

    sprintf (path, "/%s/global_properties/replication", octopus);
    result =
        zoo_create (zh, path, (const char *) &replication, sizeof (int),
                    &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }

    sprintf (path, "/%s/global_properties/interval", octopus);
    result = zoo_create (zh, path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }

    sprintf (path, "/%s/global_properties/interval/interval_size", octopus);
    result =
        zoo_create (zh, path, (const char *) &interval_size,
                    sizeof (unsigned long), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }
    int zero = 0;
    sprintf (path, "/%s/global_properties/interval/last_interval", octopus);
    result =
        zoo_create (zh, path, (const char *) &zero, sizeof (int),
                    &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    if (result != ZOK) {
        printf ("\nError");
        return 1;
    }

    printf ("\n Octopus registered");
    return 0;
}
