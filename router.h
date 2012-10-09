#ifndef _ROUTER_H_
#define _ROUTER_H_

#include <stdint.h>
#include <stddef.h>
#include "tree/tree.h"
#include <czmq.h>


struct _hkey_t
{
  uint64_t prefix;
  uint64_t suffix;
};


struct node_t
{
unsigned long st_piece;
unsigned long n_pieces;
char key[100];  //key is the routing_address or the subscription subject that we will accept from this node.
  int alive; //this is only used in a db_routing, all worker nodes received 
             //are assumed alive. The reason for this is that db nodes need 
             //to remain the same for each vertex despite the failures
             // so that the vertex can know which db to fix in case of failures

char bind_point[30];
};

struct hash_t
{
  struct _hkey_t hkey;
  struct node_t *node;
    RB_ENTRY (hash_t) field;
};

int
cmp_hash_t (struct hash_t *first, struct hash_t *second);


RB_HEAD (hash_rb_t, hash_t);
RB_PROTOTYPE(hash_rb_t, hash_t, field, cmp_hash_t);

typedef struct router_t router_t;
typedef struct node_t node_t;

int router_init(router_t **router,int type);

int router_destroy(router_t * router);

//if the node exists it does nothing
int router_add(router_t * router,node_t *node);

//first I need to find the exact same node I want to delete
int router_delete(router_t * router,node_t *node);

//rkey, the id adrress of the node
int router_route(router_t * router,uint64_t key, char *rkey);

int router_set_repl(router_t * router, int repl);

int router_get_repl(struct router_t * router, int *repl);

//rkey contains the addresses of the nodes
//rkey should be big enough and should check repl before
//this will only return alive nodes

//we grab the first repl number of nodes and return only the alive ones
int router_dbroute(struct router_t * router,uint64_t key, char **rkey, int *nreturned);

//finds a node that has a piece with this key
//returns null if not found
int router_fnode(struct router_t * router,char * key, node_t **node);

//result should be big enough
int node_piece(char *key, unsigned long pnumber, char * result);

int node_init(node_t **node,char *key, unsigned long n_pieces, unsigned long st_piece,char *bind_point);

// only used in a db_routing 
int node_set_alive(node_t *node, int alive);



#endif
