#include"tree/tree.h"
#include"MurmurHash/MurmurHash3.h"
#include <stddef.h>
#include<stdlib.h>
#include<string.h>
#include"router.h"
#include<stdio.h>
#include<assert.h>

//result should be big enough
int node_piece(char *key, unsigned long pnumber, char * result){
sprintf(result,"%s%u",key,pnumber);
}


RB_GENERATE (hash_rb_t, hash_t, field, cmp_hash_t);


int
cmp_hash_t (struct hash_t *first, struct hash_t *second)
{

  if (first->hkey.prefix > second->hkey.prefix)
    {
      return 1;
    }
  else
    {
      if (first->hkey.prefix < second->hkey.prefix)
	{
	  return -1;
	}
      else
	{
	  if (first->hkey.suffix > second->hkey.suffix)
	    {
	      return 1;
	    }
	  else
	    {
	      if (first->hkey.suffix < second->hkey.suffix)
		{
		  return -1;
		}
	      else
		{
		  return 0;
		}
	    }
	}
    }

}


struct router_t
{
  struct hash_rb_t hash_rb;
  int repl;  //replication, used only be the db_routing
};





int router_init(struct router_t ** router){

*router=(struct router_t *) malloc(sizeof(struct router_t));

RB_INIT(&((*router)->hash_rb));

}

int router_destroy(struct router_t * router){

struct hash_t *hash;
//deletion per node
while(hash=RB_MIN(hash_rb_t,&(router->hash_rb))){
router_delete(router,hash->node);
}

free(router);

}
//I save the key with the null char but I compute the hash without the null
int router_add(struct router_t * router,node_t *node){

char key[1000];
int iter;
for(iter=0; iter<node->n_pieces; iter++){
node_piece(node->key,iter+node->st_piece,key);


struct hash_t *hash=(struct hash_t *)malloc(sizeof(struct hash_t));
hash->node=node;

MurmurHash3_x64_128 ( (void *)key,strlen(key),0, (void *)&(hash->hkey) );

if(RB_INSERT(hash_rb_t,&(router->hash_rb),hash)!=NULL){
free(hash);
}
}
}

int router_delete(struct router_t * router,node_t *node){

if(node!=NULL){
char key[1000];
int iter;
for(iter=0; iter<node->n_pieces; iter++){
node_piece(node->key,iter+node->st_piece,key);

struct hash_t hash;
struct hash_t *result;

MurmurHash3_x64_128 ( (void *)key,strlen(key),0, (void *) &(hash.hkey) );

result=RB_FIND(hash_rb_t,&(router->hash_rb),&hash);

if(result!=NULL){
RB_REMOVE(hash_rb_t,&(router->hash_rb),result);
free(result);
}
}
free(node);
}
}
//rkey should be big enough
int router_route(struct router_t * router,char *key, char *rkey){

struct hash_t hash;
struct hash_t *result;

MurmurHash3_x64_128 ( (void *)key,strlen(key),0, (void *) &(hash.hkey) );

result=RB_NFIND(hash_rb_t,&(router->hash_rb),&hash);

if(result==NULL){
result=RB_MIN(hash_rb_t,&(router->hash_rb));
}

if(result==NULL){
printf("\nError, there are no nodes at all");
}
assert(result!=NULL);

strcpy(rkey,result->node->key);

}

int router_set_repl(struct router_t * router, int repl){
router->repl=repl;
}

int router_get_repl(struct router_t * router, int *repl){
*repl=router->repl;
}


//only used in db_routing
int node_set_alive(node_t *node, int alive){
node->alive=alive;
}

//rkey the key of the node that is responsible for the key
//rkey should be big enough and should check repl before
//this will only return alive nodes

//we grab the first repl number of nodes and return only the alive ones
int router_dbroute(struct router_t * router,char *key, char **rkey, int *nreturned){

struct hash_t hash;
struct hash_t *result;
struct hash_t *first_result;//used to identify a full circle
*nreturned=0;

MurmurHash3_x64_128 ( (void *)key,strlen(key),0, (void *) &(hash.hkey) );

result=RB_NFIND(hash_rb_t,&(router->hash_rb),&hash);

if(result==NULL){
result=RB_MIN(hash_rb_t,&(router->hash_rb));
}

if(result==NULL){
printf("\nError, there are no nodes at all");
}
assert(result!=NULL);

int all=1;
first_result=result;
//printf("\ndbroute debug key: %s",result->key);
if(result->node->alive){
strcpy(rkey[0],result->node->key);
(*nreturned)++;
}
while(1){
struct hash_t *prev_result=result;
result=RB_NEXT(hash_rb_t,&(router->hash_rb),prev_result);

//go back to the beggining
if(result==NULL){
result=RB_MIN(hash_rb_t,&(router->hash_rb));
}
//stop at a full circle or we found a repl number of nodes dead or alive
if((strcmp(result->node->key,first_result->node->key)==0)||(all==router->repl)){
break;
}

//chech whether we already picked this node
int iter;
int neww=1;
for(iter=0; iter<*nreturned; iter++){
if(0==strcmp(rkey[iter],result->node->key)){
neww=0;
break;
}
}

if(neww){

all++;  //all unique dead or alive
if(result->node->alive){

strcpy(rkey[*nreturned],result->node->key);
(*nreturned)++;
}
}
}



}


int router_fnode(struct router_t * router,char * key, node_t **node){

struct hash_t hash;
struct hash_t *result;

MurmurHash3_x64_128 ( (void *)key,strlen(key),0, (void *) &(hash.hkey) );

result=RB_FIND(hash_rb_t,&(router->hash_rb),&hash);

if(result!=NULL){
*node=result->node;
}else {
*node=NULL;
}
}


int node_init(node_t **node,char *key, unsigned long n_pieces, unsigned long st_piece){

*node=(node_t *)malloc(sizeof(node_t));
strcpy((*node)->key,key);
(*node)->n_pieces=n_pieces;
(*node)->st_piece=st_piece;

}

