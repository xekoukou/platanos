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





#include"tree/tree.h"
#include"MurmurHash/MurmurHash3.h"
#include <stddef.h>
#include<stdlib.h>
#include<string.h>
#include"router.h"
#include<stdio.h>
#include<assert.h>
#include<czmq.h>
#include"hash/khash.h"
#include"aknowledgements.h"

//result should be big enough
void
node_piece (char *key, unsigned long pnumber, char *result)
{
    sprintf (result, "%s%lu", key, pnumber);
}


RB_GENERATE (hash_rb_t, hash_t, field, cmp_hash_t);


int
cmp_hash_t (struct hash_t *first, struct hash_t *second)
{

    if (first->hkey.prefix > second->hkey.prefix) {
	return 1;
    }
    else {
	if (first->hkey.prefix < second->hkey.prefix) {
	    return -1;
	}
	else {
	    if (first->hkey.suffix > second->hkey.suffix) {
		return 1;
	    }
	    else {
		if (first->hkey.suffix < second->hkey.suffix) {
		    return -1;
		}
		else {
		    return 0;
		}
	    }
	}
    }

}


void
router_init (struct router_t **router, int type)
{

    *router = malloc (sizeof (struct router_t));

    (*router)->type = type;
    RB_INIT (&((*router)->hash_rb));
    (*router)->self = NULL;
    (*router)->repl = -1;

    nodes_init (&(*router)->nodes);


}

void
router_destroy (struct router_t *router)
{

    struct hash_t *hash;
//deletion per node
    while ((hash = RB_MIN (hash_rb_t, &(router->hash_rb)))) {
	router_delete (router, hash->node);
    }

    free (router);

}

//I save the key with the null char but I compute the hash without the null
//return 0 if the element already exists and thus the new node is not inserted
int
router_add (struct router_t *router, node_t * node)
{

    char key[1000];

    struct hash_t *hash[node->n_pieces];

    int iter;
    for (iter = 0; iter < node->n_pieces; iter++) {
	node_piece (node->key, iter + node->st_piece, key);


	hash[iter] = malloc (sizeof (struct hash_t));
	hash[iter]->node = node;

	MurmurHash3_x64_128 ((void *) key, strlen (key), 0,
			     (void *) &(hash[iter]->hkey));

	if (RB_INSERT (hash_rb_t, &(router->hash_rb), hash[iter]) != NULL) {
	    //delete the previous hashes
	    int siter;
	    for (siter = 0; siter < iter; siter++) {
		RB_REMOVE (hash_rb_t, &(router->hash_rb), hash[siter]);
		free (hash[siter]);
	    }
	    return 0;
	}
    }
    nodes_put (router->nodes, node);
    return 1;

}

void
router_delete (struct router_t *router, node_t * node)
{

    if (node != NULL) {
	char key[1000];
	int iter;
	for (iter = 0; iter < node->n_pieces; iter++) {
	    node_piece (node->key, iter + node->st_piece, key);

	    struct hash_t hash;
	    struct hash_t *result;

	    MurmurHash3_x64_128 ((void *) key, strlen (key), 0,
				 (void *) &(hash.hkey));

	    result = RB_FIND (hash_rb_t, &(router->hash_rb), &hash);

	    if (result != NULL) {
		RB_REMOVE (hash_rb_t, &(router->hash_rb), result);
		free (result);
	    }
	}
	nodes_delete (router->nodes, node->key);
	free (node);
    }
}

//rkey should be big enough
char *
router_route (struct router_t *router, uint64_t key)
{

    struct hash_t hash;
    struct hash_t *result;

    MurmurHash3_x64_128 ((void *) &key, sizeof (uint64_t), 0,
			 (void *) &(hash.hkey));

    result = RB_NFIND (hash_rb_t, &(router->hash_rb), &hash);

    if (result == NULL) {
	result = RB_MIN (hash_rb_t, &(router->hash_rb));
    }

    if (result == NULL) {
	return NULL;
    }

    return result->node->key;

}

void
router_set_repl (struct router_t *router, int repl)
{
    router->repl = repl;
}

void
router_get_repl (struct router_t *router, int *repl)
{
    *repl = router->repl;
}


//only used in db_routing
void
node_set_alive (node_t * node, int alive)
{
    node->alive = alive;
}

//rkey the key of the node that is responsible for the key
//rkey should be big enough and should check repl before
//this will only return alive nodes

//we grab the first repl number of nodes and return only the alive ones
void
router_dbroute (struct router_t *router, uint64_t key, char **rkey,
		int *nreturned)
{

    struct hash_t hash;
    struct hash_t *result;
    struct hash_t *first_result;	//used to identify a full circle
    *nreturned = 0;

    MurmurHash3_x64_128 ((void *) &key, sizeof (uint64_t), 0,
			 (void *) &(hash.hkey));

    result = RB_NFIND (hash_rb_t, &(router->hash_rb), &hash);

    if (result == NULL) {
	result = RB_MIN (hash_rb_t, &(router->hash_rb));
    }

    if (result == NULL) {
	*rkey = NULL;
	return;
    }

    int all = 1;
    first_result = result;
//printf("\ndbroute debug key: %s",result->key);
    if (result->node->alive) {
	strcpy (rkey[0], result->node->key);
	(*nreturned)++;
    }
    while (1) {
	struct hash_t *prev_result = result;
	result = RB_NEXT (hash_rb_t, &(router->hash_rb), prev_result);

//go back to the beggining
	if (result == NULL) {
	    result = RB_MIN (hash_rb_t, &(router->hash_rb));
	}
//stop at a full circle or we found a repl number of nodes dead or alive
	if ((strcmp (result->node->key, first_result->node->key) == 0)
	    || (all == router->repl)) {
	    break;
	}

//chech whether we already picked this node
	int iter;
	int neww = 1;
	for (iter = 0; iter < *nreturned; iter++) {
	    if (0 == strcmp (rkey[iter], result->node->key)) {
		neww = 0;
		break;
	    }
	}

	if (neww) {

	    all++;		//all unique dead or alive
	    if (result->node->alive) {

		strcpy (rkey[*nreturned], result->node->key);
		(*nreturned)++;
	    }
	}
    }


}

//returns an array of events of a specific size
//if this node already exist it creates events from the difference of their settings
//removal(0,1) is boolean indicating whether this is about removing a node or adding a node
//0 also means that there might be a change in st_piece, n_pieces


//node might literary be exists(memory wise)

//TODO remove sizee
zlist_t *
router_events (router_t * router, node_t * node, int removal)
{
    zlist_t *solution = zlist_new ();
    node_t *exists;
    int doesnt_exist = 1;

    int dif_st_piece = 0;
    int dif_n_pieces = 0;
    struct hash_t *hash;	//hash array
    int *remove;		// an array that shows if that hash is removed or added
    int size;			//the size of the array

    exists = router_fnode (router, node->key);
    if (!removal) {
//check whether the node already exists
	if (exists) {

	    dif_st_piece = node->st_piece - exists->st_piece;
	    dif_n_pieces = node->n_pieces - exists->n_pieces;
	    doesnt_exist = 0;
	}

//always positive
	if (dif_st_piece) {
	    assert (dif_n_pieces == 0);

	    hash = malloc (dif_st_piece * 2 * sizeof (struct hash_t));
	    remove = malloc (dif_st_piece * 2 * sizeof (int));
	    size = dif_st_piece * 2;

	    char key[1000];
	    int iter;
	    for (iter = 0; iter < dif_st_piece; iter++) {
		node_piece (node->key, iter + exists->st_piece, key);


		MurmurHash3_x64_128 ((void *) key, strlen (key), 0,
				     (void *) &(hash[2 * iter].hkey));

		remove[2 * iter] = 1;
		node_piece (node->key,
			    iter + exists->st_piece + exists->n_pieces, key);


		MurmurHash3_x64_128 ((void *) key, strlen (key), 0,
				     (void *) &(hash[2 * iter + 1].hkey));

		remove[2 * iter + 1] = 0;



	    }
	}

	if (dif_n_pieces) {
	    assert (dif_st_piece == 0);

	    hash = malloc (dif_n_pieces * sizeof (struct hash_t));
	    remove = malloc (dif_n_pieces * sizeof (int));
	    size = dif_n_pieces;
	    char key[1000];
	    int iter = dif_n_pieces;
	    int positive = 0;
	    if (dif_n_pieces > 0) {
		positive = 1;
	    }
	    while (iter) {
		node_piece (node->key,
			    iter + exists->st_piece + exists->n_pieces - 1,
			    key);


		MurmurHash3_x64_128 ((void *) key, strlen (key), 0, (void *)
				     &(hash
				       [(positive * 2 - 1) * iter - 1].hkey));

		remove[(positive * 2 - 1) * iter - 1] = 1 - positive;

		iter = iter - positive * 2 + 1;
	    }
	}

	if (doesnt_exist) {

	    hash = malloc (node->n_pieces * sizeof (struct hash_t));
	    remove = malloc (node->n_pieces * sizeof (int));
	    size = node->n_pieces;
	    char key[1000];
	    int iter = node->n_pieces;
	    while (iter) {
		node_piece (node->key, iter + node->st_piece - 1, key);


		MurmurHash3_x64_128 ((void *) key, strlen (key), 0, (void *)
				     &(hash[iter - 1].hkey));

		remove[iter - 1] = 0;


		iter--;


	    }




	}
    }
    else {
//the case where we remove a node
	hash = malloc (node->n_pieces * sizeof (struct hash_t));
	remove = malloc (node->n_pieces * sizeof (int));
	size = node->n_pieces;
	char key[1000];
	int iter = node->n_pieces;
	while (iter) {
	    node_piece (node->key, iter + node->st_piece - 1, key);


	    MurmurHash3_x64_128 ((void *) key, strlen (key), 0, (void *)
				 &(hash[iter - 1].hkey));

	    remove[iter - 1] = 1;


	    iter--;


	}



    }

//The creation of the events
//
//
//
//


    int iter;
    for (iter = 0; iter < size; iter++) {
	if (remove[iter] != 2) {

	    struct hash_t *forward = NULL;	//the first forward hash that is not the to be changed
	    struct hash_t *ask = NULL;	//the greatest added hash till the forward hash or NULL
	    struct hash_t *limit = NULL;	//the greatest removed hash till the forward hash with a lower limit the backward hash or NULL
	    struct hash_t *backward = NULL;	//the smallest hash that is not to be changed


//forward cannot be one of the removed hashes

	    forward = &(hash[iter]);
	    forward = RB_NFIND (hash_rb_t, &(router->hash_rb), forward);
	    while (1) {
		int br = 1;
		if (forward == NULL) {
		    break;
		}
		int triter;
		for (triter = 0; triter < size; triter++) {
		    if (cmp_hash_t (&(hash[triter]), forward) == 0) {
			assert (remove[triter] != 0);
			limit = &(hash[triter]);

			remove[triter] = 2;

			br = 0;
			break;
		    }
		}
		if (br) {
		    break;
		}
		forward = RB_NEXT (hash_rb_t, &(router->hash_rb), forward);
	    }


	    //we cross the "river"
	    if (forward == NULL) {
		forward = RB_MIN (hash_rb_t, &(router->hash_rb));
		while (1) {
		    int br = 1;
		    //the case where there are no nodes at all
		    if (forward == NULL) {
			break;
		    }
		    int triter;
		    for (triter = 0; triter < size; triter++) {
			if (cmp_hash_t (&(hash[triter]), forward) == 0) {

			    assert (remove[triter] != 0);
			    limit = &(hash[triter]);


			    remove[triter] = 2;

			    br = 0;
			    break;
			}
		    }


		    if (br) {
			break;
		    }
		    forward =
			RB_NEXT (hash_rb_t, &(router->hash_rb), forward);

		}
	    }

	    if (forward != NULL) {

		//there is no NPREV in the library so we use the forward result

		backward = RB_PREV (hash_rb_t, &(router->hash_rb), forward);
		while (1) {
		    int br = 1;	// same comments as in forward
		    if (backward == NULL) {
			break;
		    }
		    int triter;
		    for (triter = 0; triter < size; triter++) {
			if (cmp_hash_t (&(hash[triter]), backward) == 0) {
			    assert (remove[triter] != 0);
			    if (!limit) {
				limit = &(hash[triter]);

			    }
			    remove[triter] = 2;

			    br = 0;
			    break;
			}
		    }
		    if (br) {
			break;
		    }
		    backward =
			RB_PREV (hash_rb_t, &(router->hash_rb), backward);

		}

		if (backward == NULL) {
		    backward = RB_MAX (hash_rb_t, &(router->hash_rb));
		    while (1) {
			int br = 1;
			assert (backward != NULL);


			int triter;
			for (triter = 0; triter < size; triter++) {
			    if (cmp_hash_t (&(hash[triter]), backward) == 0) {
				assert (remove[triter] != 0);
				if (!limit) {
				    limit = &(hash[triter]);

				}
				remove[triter] = 2;

				br = 0;
				break;
			    }
			}


			if (br) {
			    break;
			}
			backward =
			    RB_PREV (hash_rb_t, &(router->hash_rb), backward);

		    }


		}


		if (!limit) {

		    limit = backward;
		}


		//find the rest(ie the added ones) of the hashes that are inside the interval forward<-backward
		//disable them while also finding the ask hash


		interval_t *big_interval;
		interval_init (&big_interval, &(backward->hkey),
			       &(forward->hkey));
		interval_t *small_interval = NULL;

//using the small interval to prove whether a hash is bigger or smaller to another hash according to the orientation
		int siter;
		for (siter = 0; siter < size; siter++) {
		    if (remove[siter] == 0) {

			if (interval_belongs_h
			    (big_interval, &(hash[siter].hkey))) {

			    if (small_interval == NULL) {
//happens only at the beggining
				interval_init (&small_interval,
					       &(backward->hkey),
					       &(hash[siter].hkey));

				ask = &(hash[siter]);
				remove[siter] = 2;
			    }
			    else {


				if (interval_belongs_h
				    (small_interval, &(hash[siter].hkey))) {


				    remove[siter] = 2;

				}
				else {
				    free (small_interval);
				    interval_init (&small_interval,
						   &(backward->hkey),
						   &(hash[siter].hkey));



				    ask = &(hash[siter]);
				    remove[siter] = 2;
				}

			    }
			}
		    }
		}

		if (!small_interval) {
		    free (small_interval);
		}
		free (big_interval);

		if (!ask) {

		    ask = backward;
		}
		assert (limit != ask);

		int ask_greater_limit;


		interval_t *interval;
		interval_init (&interval, &(backward->hkey), &(limit->hkey));


		if (interval_belongs_h (interval, &(ask->hkey))) {

		    ask_greater_limit = 0;
		}
		else {
		    ask_greater_limit = 0;

		}
		free (interval);

		event_t *event=NULL;

		if (!ask_greater_limit) {

//it gives
		    if (strcmp (forward->node->key, router->self->key) != 0) {
			event_init (&event, ask->hkey, limit->hkey, 1,
				    forward->node->key);
			assert (strcmp (event->key, "\0") != 0);

		    }

		}
		else {

//it receives
		    if (strcmp (forward->node->key, router->self->key) != 0) {

			event_init (&event, limit->hkey, ask->hkey, 0,
				    forward->node->key);
			assert (strcmp (event->key, "\0") != 0);

		    }
		}

		if (event) {
		    zlist_append (solution, event);

		}






	    }			//forward NULL
	    else {



		if (!exists) {

		    event_t *event;
		    struct _hkey_t start;
		    struct _hkey_t end;


		    start.prefix = 0;
		    start.suffix = 0;

		    end.prefix = 0xFFFFFFFFFFFFFFFF;
		    end.suffix = 0xFFFFFFFFFFFFFFFF;

		    event_init (&event, start, end, 0, NULL);

		    zlist_append (solution, event);
		    return solution;


		}




		free (hash);
		free (remove);

		return solution;


	    }
	}
    }

    free (hash);
    free (remove);

    return solution;


}



//the key contains only the address
node_t *
router_fnode (struct router_t * router, char *key)
{
    return nodes_search (router->nodes, key);
}


//NODES HASH is used to find a node only by its key



void
nodes_init (khash_t (nodes_t) ** nodes)
{

    *nodes = kh_init (nodes_t);
}

void
nodes_put (khash_t (nodes_t) * nodes, node_t * node)
{
    int is_missing;
    khiter_t k;
    k = kh_put (nodes_t, nodes, node->key, &is_missing);
    kh_value (nodes, k) = node;

}

//deletes the entry if present
//the node is deleted bu router_delete
void
nodes_delete (khash_t (nodes_t) * nodes, char *key)
{
    khiter_t k;
    k = kh_get (nodes_t, nodes, key);
    if (k != kh_end (nodes)) {
	kh_del (nodes_t, nodes, k);
    }


}

//returns NULL if not found
node_t *
nodes_search (khash_t (nodes_t) * nodes, char *key)
{

    khiter_t k;
    k = kh_get (nodes_t, nodes, key);
    if (k != kh_end (nodes)) {
	return kh_value (nodes, k);
    }
    return NULL;


}



void
node_init (node_t ** node, char *key, int n_pieces,
	   unsigned long st_piece, char *bind_point_nb,
	   char *bind_point_wb, char *bind_point_bl)
{

    *node = malloc (sizeof (node_t));
    strcpy ((*node)->key, key);
    (*node)->n_pieces = n_pieces;
    (*node)->st_piece = st_piece;
    strcpy ((*node)->bind_point_nb, bind_point_nb);
    strcpy ((*node)->bind_point_wb, bind_point_wb);
    strcpy ((*node)->bind_point_bl, bind_point_bl);

}

node_t *
node_dup (node_t * node)
{

    node_t *new_node = malloc (sizeof (node_t));
    memcpy (new_node, node, sizeof (node_t));
    return new_node;
}
