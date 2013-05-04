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


#include "db_on_receive.h"


void
db_on_receive_init (db_on_receive_t ** on_receive,db_balance_t *balance, int id, char *key,
                    zmsg_t * msg)
{
interval_t *interval;
    interval_minit (&((*on_receive)->interval), msg);
  //search if there is a dead on_receive
    db_on_receive_t *iter = zlist_first (balance->on_receives);
    while (iter) {
        if ((memcmp (key, iter->key, 18) == 0)&&(interval_identical(interval,iter->interval))) {
       db_on_receive_reset (iter,id);
       free(interval);
            zlist_remove (balance->don_receives, iter);
       *on_receive=iter;
        return; 
        }
        iter = zlist_next (on_receives);
    }



    *on_receive = malloc (sizeof (on_receive_t));
    (*on_receive)->interval = interval;
    (*on_receive)->un_id = id;



    (*on_receive)->m_counters = zlist_new ();
    (*on_receive)->counter = 0;
    (*on_receive)->last_time = 0;       //should only be checked if theu are miising chunkes


//make on_receive durable
//on_receive is not unique per un_id

    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    leveldb_writeoptions_set_sync (writeoptions, 1);
    unsigned char key[28];
    key[0] = 2;
    key[1] = 0;
    memcpy (key + 2, (*on_receive)->un_id, sizeof (int));
    key[6] = 0;
    int len=strlen((*on_receive)->key)+1;
    memcpy (key + 7, (*on_receive)->key,len );
   

    leveldb_writebatch_t *writebatch = leveldb_writebatch_create ();

    char **errptr = NULL;

    key[7+len] = 1;                 //the key

    leveldb_writebatch_put (writebatch,
                            key, 8,
                            (*on_receive)->key, strlen ((*on_receive)->key) + 1);

    key[7+len] = 2;                 //the start of the interval

    leveldb_writebatch_put (writebatch,
                            key, 8, (*on_receive)->interval->start, 16);


    key[7+len] = 3;                 //the end of the interval

    leveldb_writebatch_put (writebatch, key, 8, (*on_receive)->interval->end, 16);

    leveldb_write (balance->dbo->db, writeoptions, writebatch, errptr);


    assert (errptr == NULL);

    leveldb_writebatch_destroy (writebatch);

    leveldb_writeoptions_destroy (writeoptions);





}


void
db_on_receive_reset (db_on_receive_t * on_receive,int id)
{
    on_receive->un_id = id;
    zlist_destroy (&(on_receive->m_counters));
    on_receive->m_counters = zlist_new ();
    on_receive->counter = 0;
    on_receive->last_time = 0;      

}

void
db_on_receive_destroy (db_on_receive_t ** on_receive)
{

//destroy durable on_receive 
    leveldb_writeoptions_t *writeoptions = leveldb_writeoptions_create ();
    leveldb_writeoptions_set_sync (writeoptions, 1);
    unsigned char key[28];
    key[0] = 2;
    key[1] = 0;
    memcpy (key + 2, (*on_receive)->un_id, sizeof (int));
    key[6] = 0;
    int len=strlen((*on_receive)->key)+1;
    memcpy (key + 7, (*on_receive)->key,len );


    leveldb_writebatch_t *writebatch = leveldb_writebatch_create ();

    char **errptr = NULL;

    key[7+len] = 1;                 //the key

    leveldb_writebatch_delete (writebatch, key, 8);

    key[7+len] = 2;                 //the start of the interval

    leveldb_writebatch_delete (writebatch, key, 8);


    key[7+len] = 3;                 //the end of the interval

    leveldb_writebatch_delete (writebatch, key, 8);

    leveldb_write (balance->dbo->db, writeoptions, writebatch, errptr);


    assert (errptr == NULL);

    leveldb_writebatch_destroy (writebatch);
    leveldb_writeoptions_destroy (writeoptions);



    assert ((*on_receive)->m_counters != NULL);
    zlist_destroy (&((*on_receive)->m_counters));
    free (*on_receive);
    on_receive = NULL;
}


//on_receive events are not unique per id
db_on_receive_t *
db_on_receives_search (zlist_t * on_receives, int id, char *key)
{

    db_on_receive_t *iter = zlist_first (on_receives);
    while (iter) {
        if ((id == iter->un_id) && (memcmp (key, iter->key, 18) == 0)) {
            return iter;
        }
        iter = zlist_next (on_receives);
    }

    return NULL;

}

void db_on_receives_dead(db_balance_t *balance, node_t *node){

db_on_receive_t *on_receive = zlist_first (balance->on_receives);

    while (on_receive) {

        if (strcmp (on_receive->key, node->key) == 0) {
            zlist_remove (balance->on_receives, on_receive);
            zlist_append (balance->don_receives, on_receive);
        }
        on_receive = zlist_next (balance->on_receives);
    }

}
