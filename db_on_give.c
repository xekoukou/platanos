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


#include "db_on_give.h"

#define NEW_CHUNK    "\002"


void
db_on_give_init (db_on_give_t ** on_give, db_balance_t * balance, char *key,
                 interval_t * interval, int un_id)
{
    *on_give = malloc (sizeof (on_give_t));
    memcpy ((*on_give)->key, key, strlen (key) + 1);
    (*on_give)->rec_counter = 0;
    (*on_give)->last_counter = 0;
    (*on_give)->state = 0;
    (*on_give)->pending_confirmations = 0;
    (*on_give)->un_id = un_id;
    (*on_give)->last_time = zclock_time ();
    (*on_give)->interval = interval;

    memset((*on_give)->unc_iter,0,50);        //NULL represents no unc chunkes

    (*on_give)->responce = zmsg_new ();
    zframe_t *frame =
        zframe_new (balance->self_key, strlen (balance->self_key));
    zmsg_add ((*on_give)->responce, frame);
    frame = zframe_new (NEW_CHUNK, 1);
    zmsg_add ((*on_give)->responce, frame);
    zmsg_add ((*on_give)->responce,
              zframe_new (&((*on_give)->un_id), sizeof (int)));

//make on_give durable
    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    leveldb_writeoptions_set_sync (writeoptions, 1);
    unsigned char key[10];
    key[0] = 1;
    key[1] = 0;
    memcpy (key + 2, (*on_give)->un_id, sizeof (int));
    key[6] = 0;

    leveldb_writebatch_t *writebatch = leveldb_writebatch_create ();

    char **errptr = NULL;

    key[7] = 1;                 //the key

    leveldb_writebatch_put (writebatch,
                            key, 8,
                            (*on_give)->key, strlen ((*on_give)->key) + 1);

    key[7] = 2;                 //the start of the interval

    leveldb_writebatch_put (writebatch,
                            key, 8, (*on_give)->interval->start, 16);


    key[7] = 3;                 //the end of the interval

    leveldb_writebatch_put (writebatch, key, 8, (*on_give)->interval->end, 16);

    leveldb_write (balance->dbo->db, writeoptions, writebatch, errptr);


    assert (errptr == NULL);

    leveldb_writebatch_destroy (writebatch);

    leveldb_writeoptions_destroy (writeoptions);

//create iterator
    leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
    leveldb_readoptions_set_fill_cache (read_options, 0);
    levelb_iterator_t *iter =
        leveldb_crate_iterator (balance->dbo->db, read_options);
    leveldb_iter_seek_to_first (iter);

    (*on_give)->iter = iter;
    (*on_give)->readoptions = readoptions;
}


void
db_on_give_reset (db_on_give_t * on_give)
{
    on_give->rec_counter = 0;
    on_give->last_counter = 0;
    on_give->state = 0;
    on_give->pending_confirmations = 0;
    on_give->last_time = zclock_time ();

    on_give->unc_iter = NULL;        //NULL represents no unc chunkes
leveldb_iter_seek(on_give->iter, on_give->unc_iter, strlen(on_give->unc_iter));

}



void
db_on_give_destroy (db_on_give_t ** on_give,db_balance_t *balance)
{
    free ((*on_give)->interval);
    zmsg_destroy (&(*on_give)->responce);

//destroy durable on_give 
    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    leveldb_writeoptions_set_sync (writeoptions, 1);
    unsigned char key[10];
    key[0] = 1;
    key[1] = 0;
    memcpy (key + 2, (*on_give)->un_id, sizeof (int));
    key[6] = 0;

    leveldb_writebatch_t *writebatch = leveldb_writebatch_create ();

    char **errptr = NULL;

    key[7] = 1;                 //the key

    leveldb_writebatch_delete (writebatch, key, 8);

    key[7] = 2;                 //the start of the interval

    leveldb_writebatch_delete (writebatch, key, 8);


    key[7] = 3;                 //the end of the interval

    leveldb_writebatch_delete (writebatch, key, 8);

    leveldb_write (balance->dbo->db, writeoptions, writebatch, errptr);


    assert (errptr == NULL);

    leveldb_writebatch_destroy (writebatch);
    leveldb_writeoptions_destroy (writeoptions);



    leveldb_readoptions_destroy ((*on_give)->readoptions);
    leveldb_iter_destroy ((*on_give)->iter);
    free (*on_give);
    on_give = NULL;

}


void
db_on_gives_dead (db_balance_t * balance, node_t * node)
{
//we dont update the timer, no need
    on_give_t *iter = zlist_first (balance->on_gives);

    while (iter) {
        if (strcmp (node->key, iter->key) == 0) {
            zlist_remove (balance->on_gives, iter);
            db_on_gives_reset(iter);
            zlist_append (balance->don_gives, iter);
        }
        iter = zlist_next (on_gives);
    }

}

//linear search
//since you give the id, it is unique, thus no need to search the key as well
on_give_t *
db_on_gives_search_id (zlist_t * db_on_gives, int id)
{

    on_give_t *iter = zlist_first (on_gives);
    while (iter) {
        if (id == iter->un_id) {
            return iter;
        }
        iter = zlist_next (on_gives);
    }

    return NULL;
}

//we use this fuction when we add ourselves
//we check if a node has been deleted and destroy the on_give/on_receive if necessary

void db_on_gives_load(db_balance_t *balance){

unsigned char key[10];
    key[0] = 1;
    key[1] = 0;


//create iterator
    leveldb_readoptions_t *readoptions = leveldb_readoptions_create ();
    leveldb_readoptions_set_fill_cache (read_options, 0);
    levelb_iterator_t *iter =
        leveldb_create_iterator (balance->dbo->db, read_options);
    leveldb_iter_seek (iter,key,2);

    while(1){
size_t klen;
char *temp = leveldb_iter_key(iter,&klen);
if(temp[0]!=1){
break;
}
int un_id;
char key[18];
struct _hkey_t start;
struct _hkey_t stop;

memcpy(&un_id,temp+2,sizeof(int));
temp= leveldb_iter_value(iter, klen);
memcpy(key,temp,klen);
leveldb_iter_next(iter);
temp= leveldb_iter_value(iter, klen);
memcpy(&start,temp,klen);

leveldb_iter_next(iter);
temp= leveldb_iter_value(iter, klen);
memcpy(&start,temp,klen);

temp = leveldb_iter_key(iter,&klen);
assert(temp[0]==1);

interval_t *interval;
interval_init(&interval, &start, &stop);

db_on_give_t *on_give;
db_on_give_init (&on_give,balance,key,
                      interval,un_id);

//check the existance of the node
node_t *node=nodes_search(balance->router->nodes,key);
if(node){

//check if dead
if(node->alive){
zlist_add(balance->on_gives,on_give);
db_balance_update_give_timer (balance, on_give);
}else{
zlist_add(balance->don_gives,on_give);
}
}else{
//clean data
//TODO WRITEBATCH
            while (leveldb_iter_valid (on_give->iter)) {
                size_t kklen;
                char kkey[50];
                temp = leveldb_iter_key (on_give->iter, &kklen);
                assert (kklen <= 50);
                memcpy (kkey, temp, strlen (temp));
                if (interval_belongs (on_give->interval, kkey)) {
                    vertex_db_destroy (balance->dbo, on_give->iter);
                }
                else {
                    vertex_iter_next (on_give->iter);
                }
            }


db_on_give_destroy (on_give,balance);
}

leveldb_iter_next(iter);
}



//TODO destroy iterator


}
