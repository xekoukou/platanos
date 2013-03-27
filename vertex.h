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




#ifndef _OCTOPUS_VERTEX_H_
#define _OCTOPUS_VERTEX_H_
#include"hash/khash.h"


//the key of the vertex is saved by the hash 

struct vertex_t;
typedef struct vertex_t vertex_t;


KHASH_MAP_INIT_INT64 (vertices, vertex_t);

//you dont (de/)malloc for the struct itself
void vertex_init (vertex_t * vertex);

void vertex_destroy(vertex_t *vertex);

vertex_t * vertex_dup(vertex_t *vertex);

//the old data should not be changed
vertex_t * vertex_deserialize(zframe_t *frame);

zframe_t * vertex_serialize(vertex_t *vertex);

#endif
