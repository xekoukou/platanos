#ifndef _OCTOPUS_VERTEX_H_
#define _OCTOPUS_VERTEX_H_
#include"btree/kbtree.h"

typedef struct
{
    uint64_t key;

} vertex_t;

#endif

#define cmp_vertex_t(a, b) (((a).key > (b).key) - ((a).key < (b).key))
KBTREE_INIT (vertices, vertex_t, cmp_vertex_t);
