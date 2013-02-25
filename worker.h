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



#ifndef OCTOPUS_WORKER_H_
#define OCTOPUS_WORKER_H_

#include"on_give.h"
#include"on_receive.h"
#include"update.h"
#include"balance.h"
#include"events.h"
#include"nodes.h"
#include"actions.h"
#include"zookeeper.h"
#include"sleep.h"




//TODO I havent yet provided a destructor function
struct worker_t
{
    zhandle_t *zh;
    oconfig_t *config;
    char *id;			//comp_name +res_name
    char *res_name;
    char *comp_name;
    int64_t bnext_time;
    int64_t snext_time;
    int is_it_sleep;
    int64_t next_time;
};

typedef struct worker_t worker_t;

void worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config,
		  char *comp_name, char *res_name);


void worker_update_timeout (worker_t * worker, int new_next_time,
			    int is_it_sleep, void *wake_nod);



//arg is a const integer  
//great care not to change that integer
//it is used in the sub and dealer socket (subscription, identity)
//max 1000 workers per computer
//worker ids are from 0 till size-1
void *worker_fn (void *arg);


#endif
