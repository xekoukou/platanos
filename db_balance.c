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


#include "db_balance.h"

#define ON_TIMEOUT 4000
#define COUNTER_SIZE 1000       /* 1000 vertices per chunk */
#define NEW_INTERVAL "\001"
#define NEW_CHUNK    "\002"
#define INTERVAL_RECEIVED "\003"
#define CONFIRM_CHUNK    "\004"
#define MISSED_CHUNKES    "\005"
#define MAX_PENDING_CONFIRMATIONS 8


void
db_balance_init (db_balance_t ** balance, dbo_t * dbo,
                 void *router_bl, void *self_bl, char *key)
{

    *balance = malloc (sizeof (db_balance_t));
    (*balance)->dbo = dbo;
    (*balance)->router_bl = router_bl;
    (*balance)->self_bl = self_bl;
    memset ((*balance)->self_key, 0, 16);
    strcpy ((*balance)->self_key, key);
    intervals_init (&((*balance)->intervals));
    intervals_init (&((*balance)->locked_intervals));

}

void
db_balance_clear_timer (db_balance_t * balance)
{
    balance->next_time = -1;
}

//update after an event to a specific on_give
void
db_balance_update_give_timer (db_balance_t * balance, db_on_give_t * on_give)
{

    if ((on_give->last_time + ON_TIMEOUT < balance->next_time)
        || (balance->next_time < 0)) {
        balance->next_time = on_give->last_time + ON_TIMEOUT;
    }
}

//update after an event to a specific on_receive
void
db_balance_update_receive_timer (balance_t * balance, db_on_receive_t * on_receive)
{

    if ((on_receive->last_time + ON_TIMEOUT < balance->next_time)
        || (balance->next_time < 0)) {
        balance->next_time = on_receive->last_time + ON_TIMEOUT;
    }
}

//position the iterator to the correct position first
void
db_balance_send_next_chunk (db_balance_t * balance, db_on_give_t * on_give,
                         zframe_t * address)
{


    on_give->pending_confirmations++;

    khint_t hiter;
    uint64_t key;
    vertex_t *vertex;
    uint64_t counter = on_give->last_counter + 1;
    uint64_t vertices = 0;


        zmsg_t *responce_dup = zmsg_dup (on_give->responce);
        zframe_t *frame = zframe_new (&counter, sizeof (uint64_t));
        zmsg_add (responce_dup, frame);

        while(leveldb_iter_valid(on_give->iter)){
size_t klen;
char *iter_key=leveldb_iter_key(on_give->iter,&klen);
memcpy(&key,iter_key,strlen(iter_key));

            if (interval_belongs (on_give->interval, key)) {
                vertices++;
               
                //add it
vertex_db_to_msg(responce_dup,iter);
                if (vertices == COUNTER_SIZE) {
                    zframe_t *address_dup = zframe_dup (address);
                    zmsg_wrap (responce_dup, address_dup);
                    zmsg_send (&responce_dup, balance->router_bl);
                    on_give->last_counter++;
                    break;
                }
            }
        }
        if (vertices < COUNTER_SIZE) {
            //this is the last address frame
            zframe_t *address_dup = zframe_dup (address);
            zmsg_wrap (responce_dup, address_dup);

            zmsg_send (&responce_dup, balance->router_bl);

//zero counter /last msgs also gives the total/last counter 
            counter = 0;
            frame = zframe_new (&counter, sizeof (uint64_t));
            zmsg_add (on_give->responce, frame);
            frame = zframe_new (&(on_give->last_counter), sizeof (uint64_t));
            zmsg_add (on_give->responce, frame);


            zmsg_wrap (on_give->responce, address);
            zmsg_send (&(on_give->responce), balance->router_bl);

//state set to 1
//and update clock
            on_give->state = 1;

        }
        else {
            zframe_destroy (&address);

        }
        on_give->last_time = zclock_time ();
        balance_update_give_timer (balance, on_give);

}

void
db_balance_interval_received (db_balance_t * balance, zmsg_t * msg,
                           zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    db_on_give_t *on_give = db_on_gives_search_id (balance->on_gives, id);
    zmsg_destroy (&msg);

    fprintf (stderr,
             "\n%s:balance:\nAn INTERVAL_RECEIVED confirmation has arrived for the db_on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->key);


    if (on_give->last_counter > 0) {
//duplicate confirmation
        zframe_destroy (&id_frame);
        zframe_destroy (&address);
        return;
    }

    on_give->state = 2;
    
    while(on_give->pening_confirmations < MAX_PENDING_CONFIRMATIONS) {
        balance_send_next_chunk (balance, on_give, address);
    }

}

void
db_balance_confirm_chunk (db_balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    db_on_give_t *on_give = db_on_gives_search_id (balance->on_gives, id);

    //delete the vertices that have been verified  
    fprintf (stderr,
             "\n%s:balance:\nA CONFIRM_CHUNK confirmation has arrived for the db_on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->key);

    zframe_t *frame = zmsg_pop (msg);
    uint64_t counter;
    memcpy (&counter, zframe_data (frame), sizeof (uint64_t));

//save the position of the iterator
size_t saved_klen;
char saved_key[50];
char *temp=leveldb_iter_key(iter,&saved_klen);
assert(saved_klen<=50);
memcpy(saved_key,temp,saved_klen);
leveldb_iter_seek(on_give->iter,on_give->unc_iter,strlen(on_give->unc_iter)+1);


//a counter zero represents the end of the transfer
    if (counter != 0) {
        uint64_t diff;
        diff = counter - on_give->rec_counter;
        on_give->rec_counter = counter;
        if (diff > 0) {
            int i;
            for (i = 0; i < diff * COUNTER_SIZE; i++) {
            size_t klen;
            char key[50];
temp=leveldb_iter_key(iter,&klen);
assert(klen<=50);
memcpy(key,temp,strlen(temp));
while(1){
            if (interval_belongs (on_give->interval, key)) {
vertex_db_destroy(balance->dbo,on_give->iter);
break;
}else{
vertex_iter_next(on_give->iter);
}}
            }
        }
        for (i = 0; i < diff; i++) {
            on_give->pending_confirmations--;
//send_next_chunk
            if (on_give->pending_confirmations < MAX_PENDING_CONFIRMATIONS) {
                balance_send_next_chunk (balance, on_give, address);
            }
        }
//return the iterator to its previous position
leveldb_iter_seek(on_give->iter,saved_key,strlen(saved_key)+1);

    }
    else {

while(leveldb_iter_valid(on_give->iter)){
            size_t klen;
            char key[50];
temp=leveldb_iter_key(iter,&klen);
assert(klen<=50);
memcpy(key,temp,strlen(temp));
            if (interval_belongs (on_give->interval, key)) {
vertex_db_destroy(balance->dbo,on_give->iter);
}else{
vertex_iter_next(on_give->iter);
}}


        //remove the event form the event list
        zlist_remove (balance->on_gives, on_give);
        db_on_give_destroy (&on_give);

    }
    zmsg_destroy (&msg);
}



void
db_balance_missed_chunkes (db_balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    db_on_give_t *on_give = on_gives_search_id (balance->on_gives, id);

    fprintf (stderr,
             "\n%s:balance:\nA MISSED_CHUNKES confirmation has arrived for the db_on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->key);


    //send the missed pieces
    uint64_t diff = 0;
    while (zmsg_size (msg)) {
        uint64_t counter;
        frame = zmsg_pop (msg);
        memcpy (&counter, zframe_data (frame), sizeof (uint64_t));

        diff = counter - on_give->rec_counter;

        if (diff > 0) {

            int i;

            if (diff > 1) {
//some chunks were cofirmed, send more then

                for (i = 0; i < diff - 1; i++) {
                    on_give->pending_confirmations--;
//send_next_chunk
                    if (on_give->pending_confirmations <
                        MAX_PENDING_CONFIRMATIONS) {
                        balance_send_next_chunk (balance, on_give, address);
                    }
                }
                on_give->rec_counter = counter - 1;

//save the position of the iterator
size_t saved_klen;
char saved_key[50];
char *temp=leveldb_iter_key(iter,&saved_klen);
assert(saved_klen<=50);
memcpy(saved_key,temp,saved_klen);
leveldb_iter_seek(on_give->iter,on_give->unc_iter,strlen(on_give->unc_iter)+1);

            int i;
            for (i = 1; i < diff-1 * COUNTER_SIZE; i++) {
            size_t klen;
            char key[50];
temp=leveldb_iter_key(iter,&klen);
assert(klen<=50);
memcpy(key,temp,strlen(temp));
while(1){
            if (interval_belongs (on_give->interval, key)) {
vertex_db_destroy(balance->dbo,on_give->iter);
break;
}else{
vertex_iter_next(on_give->iter);
}}
            }


}

//send the missing chunk
balance_send_next_chunk (balance, on_give, address);


//return the iterator to its previous position
leveldb_iter_seek(on_give->iter,saved_key,strlen(saved_key)+1);

        }
    }


    zmsg_destroy (&msg);

}

void
balance_new_chunk (balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    char key[17] = { 0 };
    memcpy (key, zframe_data (address), zframe_size (address));

    on_receive_t *on_receive =
        on_receives_search (balance->on_receives, id, key);

    fprintf (stderr,
             "\n%s:balance\nA NEW_CHUNK has arrived for the db_on_receive event with id:%d and giving key:%s",
             balance->self_key, on_receive->un_id, on_receive->key);

    zmsg_t *responce = zmsg_new ();
    zframe_t *frame =
        zframe_new (balance->self_key, strlen (balance->self_key));
    zmsg_add (responce, frame);
    frame = zframe_new (CONFIRM_CHUNK, 1);
    zmsg_add (responce, frame);
    zmsg_add (responce, id_frame);




    frame = zmsg_pop (msg);

    uint64_t counter;
    memcpy (&counter, zframe_data (frame), sizeof (uint64_t));
    zframe_destroy(&frame);

//update missed_chunkes

    if (counter == 0) {
//EOTransm. signal
//if there are missing chunks request them,otherwise free the event 

        uint64_t total_counter;
        frame = zmsg_pop (msg);
        memcpy (&total_counter, zframe_data (frame), sizeof (uint64_t));
        zframe_destroy(&frame);

//check if there where more chunks than the ones received           
//add them with the rest missing if necessary
        if ((total_counter != on_receive->counter)) {

            uint64_t i;
            for (i = on_receive->counter + 1; i <= total_counter; i++) {
                uint64_t *c = malloc (sizeof (uint64_t));
                *c = i;
                zlist_append (on_receive->m_counters, &c);
            }

        }





    }
    else {

        if (counter > on_receive->counter + 1) {

            uint64_t i;
            for (i = on_receive->counter + 1; i < counter; i++) {
                uint64_t *c = malloc (sizeof (uint64_t));
                *c = i;
                zlist_append (on_receive->m_counters, &c);
            }



        }


    }

//If there are missed chunkes, a request for them is done in balance_lazy_pirate
//that is why we update the clock

    on_receive->last_time = zclock_time ();
    balance_update_receive_timer (balance, on_receive);
    if ((counter == 0)
        && (zlist_size (on_receive->m_counters)
            == 0)) {

        frame = zframe_new (&counter, sizeof (uint64_t));
        zmsg_add (responce, frame);
        zmsg_wrap (responce, address);
        zmsg_send (&responce, balance->router_bl);
//add the interval with the others
        intervals_add (balance->intervals, on_receive->interval);
        intervals_print (balance->intervals);

        //destroy on_receive
        zlist_remove (balance->on_receives, on_receive);
        on_receive_destroy (&on_receive);

        intervals_t *cintervals = router_current_intervals (update->router, node);

    db_balance_init_gives (balance,cintervals);


        zmsg_destroy (&msg);
    }
    else {

        if (counter < on_receive->counter + 1) {

//check whether it is one of the missed chunkes

            uint64_t *c = zlist_first (on_receive->m_counters);
                if (*c == counter) {

                    frame = zmsg_first (msg);
                    while (frame) {
vertex_db_msg_to_db(balance->dbo,msg);

                        frame = zmsg_first (msg);
                    }

                    zlist_remove (on_receive->m_counters, c);
                }

        }
        else {
//get the data
                    frame = zmsg_first (msg);
                    while (frame) {
vertex_db_msg_to_db(balance->dbo,msg);

                        frame = zmsg_first (msg);
                    }


           //update last counter
            on_receive->counter = counter;
        }

//send responce only if it is less than the  min lost chunk or if there are no missed_chunkes
        uint64_t *c = zlist_first (on_receive->m_counters);
        if (c == NULL) {



            frame = zframe_new (&counter, sizeof (uint64_t));
            zmsg_add (responce, frame);
            zmsg_wrap (responce, address);
            zmsg_send (&responce, balance->router_bl);
        }
        else {
            if (*c < counter) {

//since we only send confirmations in order, but receive some times out of order, we need to send the latest confirmation
                uint64_t co = *c - 1;
                frame = zframe_new (&co, sizeof (uint64_t));
                zmsg_add (responce, frame);
                zmsg_wrap (responce, address);
                zmsg_send (&responce, balance->router_bl);


            }
        }
        zmsg_destroy (&msg);
    }






}

void db_balance_new_interval
    (db_balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    char key[17] = {
        0
    };
    memcpy (key, zframe_data (address), zframe_size (address));
    db_on_receive_t *on_receive =
        db_on_receives_search (balance->on_receives, id, key);
    if (on_receive) {
//duplicate new_interval
        zmsg_destroy (&msg);
    }
    else {


        db_on_receive_t *on_receive;
        db_on_receive_init (&on_receive, id, key, msg);
        zlist_append (balance->on_receives, on_receive);
        fprintf (stderr,
                 "\n%s:balance\nA NEW_INTERVAL has arrived for the db_on_receive event with id:%d and giving key:%s",
                 balance->self_key, on_receive->un_id, on_receive->key);
    }

//send confirmation
    zmsg_t *responce = zmsg_new ();
    zframe_t *frame = zframe_new (balance->self_key,
                                  strlen (balance->self_key));
    zmsg_add (responce, frame);
    frame = zframe_new (INTERVAL_RECEIVED, 1);
    zmsg_add (responce, frame);
    zmsg_add (responce, id_frame);
    zmsg_wrap (responce, address);
    zmsg_send (&responce, balance->router_bl);


}

void
db_balance_new_msg (db_balance_t * balance, zmsg_t * msg)
{

    //destroying the zero frame
    zframe_t *zero = zmsg_pop (msg);
    zframe_destroy (&zero);
    zframe_t *address = zmsg_pop (msg);
    zframe_t *type_fr = zmsg_pop (msg);
    if (memcmp (INTERVAL_RECEIVED, zframe_data (type_fr), 1) == 0) {
        db_balance_interval_received (balance, msg, address);
    }
    else {
        if (memcmp (CONFIRM_CHUNK, zframe_data (type_fr), 1) == 0) {
            db_balance_confirm_chunk (balance, msg, address);
        }
        else {
            if (memcmp (MISSED_CHUNKES, zframe_data (type_fr), 1) == 0) {
                db_balance_missed_chunkes (balance, msg, address);
            }
            else {
                if (memcmp (NEW_CHUNK, zframe_data (type_fr), 1) == 0) {
                    db_balance_new_chunk (balance, msg, address);
                }
                else {
                    if (memcmp (NEW_INTERVAL, zframe_data (type_fr), 1) == 0) {
                        db_balance_new_interval (balance, msg, address);
                    }
                    else {
                        assert (0 == 1);
                    }
                }

            }

        }

    }

    zframe_destroy (&type_fr);
}







void
db_balance_lazy_pirate (db_balance_t * balance)
{

    int64_t time = zclock_time ();
    balance_clear_timer (balance);
//on_receives
    fprintf (stderr, "\ndb:%s:balance:Inside lazy pirate\n", balance->self_key);
    db_on_receive_t *iter = zlist_first (balance->on_receives);
    while (iter) {
        if (zlist_size (iter->m_counters) != 0) {
            if (time - iter->last_time > ON_TIMEOUT) {
//request missing chunkes
                fprintf (stderr,
                         "\ndb:%s:balance:Requesting missing chunkes from worker %s\n for action  with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key,
                         iter->key,
                         iter->interval->start.prefix,
                         iter->interval->start.suffix,
                         iter->interval->end.prefix, iter->interval->end.suffix);
                zmsg_t *responce = zmsg_new ();
                zframe_t *frame = zframe_new (balance->self_key,
                                              strlen (balance->self_key));
                zmsg_add (responce, frame);
                frame = zframe_new (MISSED_CHUNKES, 1);
                zmsg_add (responce, frame);
                frame = zframe_new (&(iter->un_id), sizeof (int));
                zmsg_add (responce, frame);
                int *counter = zlist_first (iter->m_counters);
                while (counter) {
                    frame = zframe_new (counter, sizeof (uint64_t));
                    zmsg_add (responce, frame);
                    counter = zlist_next (iter->m_counters);
                }

                frame =
                    zframe_new (iter->key, strlen (iter->key));
                zmsg_wrap (responce, frame);
                zmsg_send (&responce, balance->router_bl);
                iter->last_time = time;
                balance_update_receive_timer (balance, iter);
            }
        }
        iter = zlist_next (balance->on_receives);
    }



//on_gives

    on_give_t *siter = zlist_first (balance->on_gives);
    while (siter) {

        if (siter->state == 0) {

            fprintf (stderr,
                     "\ndb:%s:balance: Sending NEW_INTERVAL msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                     balance->self_key,
                     siter->key,
                     siter->start.prefix,
                     siter->start.suffix,
                     siter->end.prefix, siter->event->end.suffix);
            zmsg_t *msg = zmsg_new ();
            zframe_t *frame = zframe_new (balance->self_key,
                                          strlen (balance->self_key));
            zmsg_add (msg, frame);
            frame = zframe_new (NEW_INTERVAL, 1);
            zmsg_add (msg, frame);
            frame = zframe_new (&(siter->un_id), sizeof (int));
            zmsg_add (msg, frame);
            frame =
                zframe_new (&(siter->interval->start), sizeof (struct _hkey_t));
            zmsg_add (msg, frame);
            frame = zframe_new (&(siter->interval->end), sizeof (struct _hkey_t));
            zmsg_add (msg, frame);
            frame = zframe_new (siter->key, strlen (siter->event->key));
            zmsg_wrap (msg, frame);
            zmsg_send (&msg, balance->router_bl);
        }
        else {
            if (siter->state == 1) {

               fprintf (stderr,
                         "\ndb:%s:balance:Sending EOT msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key,
                         siter->key,
                         siter->interval->start.prefix,
                         siter->interval->start.suffix,
                         siter->interval->end.prefix, siter->interval->end.suffix);
                zmsg_t *msg = zmsg_new ();
                zframe_t *frame = zframe_new (balance->self_key,
                                              strlen (balance->self_key));
                zmsg_add (msg, frame);
                frame = zframe_new (NEW_CHUNK, 1);
                zmsg_add (msg, frame);
                frame = zframe_new (&(siter->un_id), sizeof (int));
                zmsg_add (msg, frame);
                uint64_t counter = 0;
                frame = zframe_new (&counter, sizeof (uint64_t));
                zmsg_add (msg, frame);
                frame =
                    zframe_new (&
                                (siter->key),
                                strlen (siter->key));
                zmsg_wrap (msg, frame);
                zmsg_send (&msg, balance->router_bl);
            }
            else {

                fprintf (stderr,
                         "\ndb:%s:balance:Sending next_chunk to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key,
                         siter->key,
                         siter->interval->start.prefix,
                         siter->interval->start.suffix,
                         siter->interval->end.prefix, siter->interval->end.suffix);
                zframe_t *address = zframe_new (&(siter->key),
                                                strlen (siter->key));
                balance_send_next_chunk (balance, siter, address);
            }
        }

        fprintf (stderr,
                 "\ndb:%s:balance:Updating on_give timer", balance->self_key);
        siter->last_time = time;
        balance_update_give_timer (balance, siter);
        siter = zlist_next (balance->on_gives);
    }
}



void  db_balance_init_gives (db_balance_t *balance, intervals_t *cintervals){


}
