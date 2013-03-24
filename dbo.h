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






#ifndef _OCTOPUS_DBO_H_
#define _OCTOPUS_DBO_H_


//leveldb
#include<leveldb/c.h>
#include<leveldb/c.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>


typedef struct
{
    leveldb_t *db;
    leveldb_options_t *options;
    leveldb_readoptions_t *readoptions;
    leveldb_writeoptions_t *writeoptions;
    char *location;

} dbo_t;


//id is the address/id of the thread/node
void dbo_init (dbo_t ** dbo);

//sleep a few seconds after
void dbo_close (dbo_t * dbo);

void dbo_open (dbo_t * dbo, char *location);

void dbo_destroy (dbo_t ** dbo);

#endif
