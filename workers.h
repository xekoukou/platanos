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

#ifndef OCTOPUS_WORKERS_H_
#define OCTOPUS_WORKERS_H_

#include<pthread.h>
#include "zookeeper.h"


struct ozookeeper_t;

struct workers_t
{
    pthread_t *pthread;
    char **id;                  //has null at the end
    int size;
};

typedef struct workers_t workers_t;



void workers_init (workers_t ** workers, struct ozookeeper_t *ozookeeper);

void workers_monitor (workers_t * workers);


#endif
