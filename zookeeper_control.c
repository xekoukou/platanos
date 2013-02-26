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

    struct Stat stat;

    char config[8][1000];

    printf ("\nWhat is the name of the octopus?");
    scanf ("%s", config[6]);


    int db;
    printf ("\n Is this node a DB node(1) or a worker node(0)?");
    scanf ("%d", &db);


    printf
        ("\nPlease, provide the connecting points (ip:port,ip:port) to the zookeeper ensemple");

    scanf ("%s", config[0]);


    printf
        ("A unique computer name, common across all resources and configs of this computer:");

    scanf ("%s", config[2]);


    printf ("A unique to this computer name for the resource:");

    scanf ("%s", config[3]);


    printf
        ("\nDo you want to change the starting piece of this worker(1) or the number of pieces?");

    int c_st_piece;

    scanf ("%d", &c_st_piece);

    if (!c_st_piece) {
        printf
            ("The number of 'chunks' in the hash ring that will be this resources responsibility:");

        scanf ("%s", config[4]);

    }


    zhandle_t *zh = zookeeper_init (config[0], global_watcher, 3000, 0, 0, 0);

//set the right path based on db
    char root[1000];
    if (db) {
        strcpy (root, "db_nodes");
    }
    else {
        strcpy (root, "worker_nodes");
    }
    int result;
    char path[1000];
    sprintf (path, "/%s/computers/%s/%s/%s", config[6], config[2], root,
             config[3]);

    result = zoo_exists (zh, path, 0, &stat);

    if (result == ZNONODE) {
        printf ("\nThe octopus you specified doesnt exist");
        return 0;
    }


    if (ZOK == result) {

        if (!c_st_piece) {
            int n_pieces = atoi (config[4]);
            sprintf (path, "/%s/computers/%s/%s/%s/n_pieces", config[6],
                     config[2], root, config[3]);
            result = zoo_set (zh, path, (char *) &n_pieces, sizeof (int), -1);
            if (ZOK == result) {
                printf ("\n all done smouthly");
                return 0;
            }
            else {
                printf ("\n the changes could not be done, exiting");
                return 0;

            }
        }
        else {
            unsigned long st_piece = 9998;
            int buffer_len = sizeof (unsigned long);
            sprintf (path, "/%s/computers/%s/%s/%s/st_piece", config[6],
                     config[2], root, config[3]);
            result =
                zoo_get (zh, path, 0, (const char *) &st_piece, &buffer_len,
                         &stat);
//I didnt deallocate the stat object
//but a memory leak is ok for this program
            if (ZOK == result) {
                st_piece++;
                printf ("\nNew st_piece is :%lu", st_piece);
                result =
                    zoo_set (zh, path, (char *) &st_piece,
                             sizeof (unsigned long), -1);

                if (ZOK == result) {
                    printf ("\n all done smouthly");
                    return 0;
                }
                else {
                    printf ("\n the changes could not be done, exiting");
                    return 0;

                }


            }
            {
                printf ("\n the changes could not be done, exiting");
                return 0;

            }

        }




    }
}
