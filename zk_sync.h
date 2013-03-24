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

#ifndef _OCTOPUS_ZK_SYNC_H_
#define _OCTOPUS_ZK_SYNC_H_

#include<zookeeper/zookeeper.h>
#include"zk_common.h"
#include<czmq.h>


struct comp_res_t
{
    char name[8];
    zlist_t *res_list;
}

void comp_res_init (comp_res_t ** comp_res, char *comp_name);
void comp_res_add (comp_res_t * comp_res, char *res_name);
int comp_res_remove (comp_res_t * comp_res, char *res_name);
void comp_res_destroy (comp_res_t ** comp_res);

typedef struct comp_res_t comp_res_t;

struct sync_t
{
    int old;
    char key[16];
    zlist_t *comp_res_list;
}

typedef struct sync_t sync_t;

void sync_init (sync_t ** sync, char *key, oz_updater_t * updater, int old);
int sync_remove_comp (sync_t * sync, char *comp_name);
int
sync_remove_res (sync_t * sync, char *comp_name, char res_name)
     int sync_ready (sync_t * sync);
