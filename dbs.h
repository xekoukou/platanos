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




#ifndef OCTOPUS_DBS_H_
#define OCTOPUS_DBS_H_

#include"zookeeper.h"
#include<stdlib.h>
#include"db.h"

struct ozookeeper_t;

struct dbs_t
{
    pthread_t *pthread;
    char **id;                  //has null at the end
    int size;
};

typedef struct dbs_t dbs_t;

void dbs_init (dbs_t ** dbs, struct ozookeeper_t *ozookeeper);


#endif