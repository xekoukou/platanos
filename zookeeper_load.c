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
#include<czmq.h>
#include"config.h"


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

    oconfig_t *fconfig;
    oconfig_init (&fconfig);

    char config[8][1000];

    oconfig_octopus (fconfig, config[6]);

    oconfig_host (fconfig, config[0]);

    zhandle_t *zh = zookeeper_init (config[0], global_watcher, 3000, 0, 0, 0);

    char path[1000];
    int result;
    int load_graph;

    sprintf (path, "/%s/load_graph", config[6], config[2]);
    Struct Stat stat;
    result = zoo_get (zh, path, 0, load_graph, sizeof (int), &stat);


    if (result != ZOK) {
        printf ("\nThere has been an Error");
        return 0;
    }

    if (load_graph == 1) {
        printf ("\nIt apears that the graph is already loaded");
        return 0;
    }
    load_graph = 1;
    result = zoo_set (zh, path, &load_graph, sizeof (int), stat.version);

    if (result == ZOK) {
        printf ("\nPlatanos %s has been signaled to load the graph", config[2]);

    }
    else {
        printf ("\nCouldnt signal platanos %s", config[2]);
    }
}
