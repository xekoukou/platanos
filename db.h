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

#ifndef OCTOPUS_DB_H_
#define OCTOPUS_DB_H_


#include"dbo.h"
#include"db_update.h"
#include"db_balance.h"
#include"nodes.h"
#include"zookeeper.h"



struct db_t
{
    zhandle_t *zh;
    oconfig_t *config;
    char *id;                   //comp_name +res_name
    char *res_name;
    char *comp_name;
};

typedef struct db_t db_t;

void db_init (db_t ** db, zhandle_t * zh, oconfig_t * config, char *comp_name,
              char *res_name);



//max 1000 dbs per computer
void *db_fn (void *arg);

#endif
