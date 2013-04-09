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
#include"compute.h"
#include"api/platanos.h"

struct compute_t;
typedef struct compute_t compute_t;

struct platanos_t;
typedef struct platanos_t platanos_t;


//TODO I havent yet provided a destructor function
struct worker_t
{
    zhandle_t *zh;
    oconfig_t *config;
    char *id;                   //comp_name +res_name
    char *res_name;
    char *comp_name;
    localdb_t *localdb;
};

typedef struct worker_t worker_t;

void worker_init (worker_t ** worker, zhandle_t * zh, oconfig_t * config,
                  char *comp_name, char *res_name, localdb_t * localdb);


int64_t worker_timeout (balance_t * balance, sleep_t * sleep,
                        int64_t platanos_next_time);


//returns the new interval or -1 on error
int worker_new_interval (worker_t * worker, localdb_t * localdb);

void worker_process_timer_events (worker_t * worker, balance_t * balance,
                                  sleep_t * sleep, platanos_t * platanos);

//max 1000 workers per computer
void *worker_fn (void *arg);


#endif
