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






#ifndef _OCTOPUS_LOCALDB_H_
#define _OCTOPUS_LOCALDB_H_


//leveldb
#include<leveldb/c.h>


typedef struct
{
    leveldb_t *db;
    leveldb_options_t *options;
    leveldb_readoptions_t *readoptions;
    leveldb_writeoptions_t *writeoptions;

} localdb_t;


//id is the address/id of the thread/node
void localdb_init (localdb_t ** localdb, char *id);

//sleep a few seconds after
void localdb_close (localdb_t * localdb);

void localdb_incr_counter (localdb_t * localdb, unsigned long counter);

unsigned long localdb_get_counter (localdb_t * localdb);

void localdb_set_interval (localdb_t * localdb, int interval);

int localdb_get_interval (localdb_t * localdb);


#endif
