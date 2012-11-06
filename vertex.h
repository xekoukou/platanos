#ifndef _OCTOPUS_VERTEX_H_
#define _OCTOPUS_VERTEX_H_
#include"hash/khash.h"


//the key of the vertex is saved by the hash 
typedef struct
{

} vertex_t;



KHASH_MAP_INIT_INT64 (vertices, vertex_t);

void vertex_init (vertex_t ** vertex);


#endif
