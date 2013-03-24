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


#include "balance.h"


#define ON_TIMEOUT 4000
#define COUNTER_SIZE 1000       /* 1000 vertices per chunk */
#define NEW_INTERVAL "\001"
#define NEW_CHUNK    "\002"
#define INTERVAL_RECEIVED "\003"
#define CONFIRM_CHUNK    "\004"
#define MISSED_CHUNKES    "\005"
#define MAX_PENDING_CONFIRMATIONS 8

void
balance_init (balance_t ** balance, worker_t * worker,
              khash_t (vertices) * hash, void *router_bl, void *self_bl,
              char *self_key)
{


    (*balance)->worker = worker;
    *balance = malloc (sizeof (balance_t));
    (*balance)->hash = hash;
    (*balance)->router_bl = router_bl;
    (*balance)->self_bl = self_bl;
    intervals_init (&((*balance)->intervals));
    (*balance)->events = zlist_new ();
    (*balance)->actions = zlist_new ();
    (*balance)->on_gives = zlist_new ();
    (*balance)->on_receives = zlist_new ();
    (*balance)->un_id = 0;
    (*balance)->next_time = -1;
    (*balance)->self_key = self_key;


}

//update after an event to a specific on_give
void
balance_clear_timer (balance_t * balance)
{
    balance->next_time = -1;
}


//update after an event to a specific on_give
void
balance_update_give_timer (balance_t * balance, on_give_t * on_give)
{

    if ((on_give->last_time + ON_TIMEOUT < balance->next_time)
        || (balance->next_time < 0)) {
        balance->next_time = on_give->last_time + ON_TIMEOUT;
    }
}

//update after an event to a specific on_receive
void
balance_update_receive_timer (balance_t * balance, on_receive_t * on_receive)
{

    if ((on_receive->last_time + ON_TIMEOUT < balance->next_time)
        || (balance->next_time < 0)) {
        balance->next_time = on_receive->last_time + ON_TIMEOUT;
    }
}

void
balance_send_next_chunk (balance_t * balance, on_give_t * on_give,
                         zframe_t * address)
{


    on_give->pending_confirmations++;

    khint_t hiter;
    uint64_t key;
    vertex_t *vertex;
    uint64_t counter = on_give->last_counter + 1;
    uint64_t vertices = 0;

    if (on_give->state == 2) {

        zmsg_t *responce_dup = zmsg_dup (on_give->responce);
        zframe_t *frame = zframe_new (&counter, sizeof (uint64_t));
        zmsg_add (responce_dup, frame);

        for (hiter = on_give->hiter + 1; hiter != kh_end (balance->hash);
             ++hiter) {
            if (!kh_exist (balance->hash, hiter))
                continue;

            key = kh_key (balance->hash, hiter);
            if (interval_belongs (on_give->interval, key)) {
                vertices++;
                vertex_init (&vertex);
                memcpy (vertex, &(kh_val (balance->hash, hiter)),
                        sizeof (vertex_t));
                //delete it from the hash
                kh_del (vertices, balance->hash, hiter);

                //add it to the list of sent vertices
                zlist_append (on_give->unc_vertices, vertex);
                //add it
                zmsg_add (responce_dup, zframe_new (vertex, sizeof (vertex_t)));
                if (vertices == COUNTER_SIZE) {
                    zframe_t *address_dup = zframe_dup (address);
                    zmsg_wrap (responce_dup, address_dup);
                    zmsg_send (&responce_dup, balance->router_bl);
                    on_give->last_counter++;
                    on_give->hiter = hiter;
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
            free (on_give->interval);

        }
        else {
            zframe_destroy (&address);

        }
        on_give->last_time = zclock_time ();
        balance_update_give_timer (balance, on_give);

    }
}

void
balance_interval_received (balance_t * balance, zmsg_t * msg,
                           zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    on_give_t *on_give = on_gives_search_id (balance->on_gives, id);
    zmsg_destroy (&msg);

    fprintf (stderr,
             "\n%s:balance:\nAn INTERVAL_RECEIVED confirmation has arrived for the on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->event->key);


    if (on_give->last_counter > 0) {
//duplicate confirmation
        zframe_destroy (&id_frame);
        zframe_destroy (&address);
        return;
    }

    on_give->state = 2;
    int i;
    for (i = 0; i < MAX_PENDING_CONFIRMATIONS; i++) {
        balance_send_next_chunk (balance, on_give, address);
    }

}


void
balance_confirm_chunk (balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    on_give_t *on_give = on_gives_search_id (balance->on_gives, id);

    //delete the vertices that have been verified  
    fprintf (stderr,
             "\n%s:balance:\nA CONFIRM_CHUNK confirmation has arrived for the on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->event->key);

    zframe_t *frame = zmsg_pop (msg);
    uint64_t counter;
    memcpy (&counter, zframe_data (frame), sizeof (uint64_t));

//a counter zero represents the end of the transfer
    if (counter != 0) {
        uint64_t diff;
        diff = counter - on_give->rec_counter;
        on_give->rec_counter = counter;
        if (diff > 0) {
            int i;
            for (i = (diff - 1) * COUNTER_SIZE; i < diff * COUNTER_SIZE; i++) {
                free (zlist_pop (on_give->unc_vertices));
            }
        }
        on_give->pending_confirmations--;
//send_next_chunk
        if (on_give->pending_confirmations < MAX_PENDING_CONFIRMATIONS) {
            balance_send_next_chunk (balance, on_give, address);
        }
    }
    else {


        while (zlist_size (on_give->unc_vertices)) {
            free (zlist_pop (on_give->unc_vertices));
        }
        //remove the event form the event list
        zlist_remove (balance->events, on_give->event);
        zlist_remove (balance->on_gives, on_give);
        on_give_destroy (&on_give);

    }
    zmsg_destroy (&msg);
}



void
balance_missed_chunkes (balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    on_give_t *on_give = on_gives_search_id (balance->on_gives, id);

    fprintf (stderr,
             "\n%s:balance:\nA MISSED_CHUNKES confirmation has arrived for the on_give event with id:%d and receiving key:%s",
             balance->self_key, on_give->un_id, on_give->event->key);

    zmsg_t *responce = zmsg_new ();
    zframe_t *frame =
        zframe_new (balance->self_key, strlen (balance->self_key));
    zmsg_add (responce, frame);
    frame = zframe_new (NEW_CHUNK, 1);
    zmsg_add (responce, frame);
    zmsg_add (responce, id_frame);

    //send the missed pieces
    uint64_t diff = 0;
    while (zmsg_size (msg)) {
        uint64_t counter;
        frame = zmsg_pop (msg);
        memcpy (&counter, zframe_data (frame), sizeof (uint64_t));
        zframe_destroy (&frame);

        diff = counter - on_give->rec_counter;

        if (diff > 0) {
            zmsg_t *responce_dup = zmsg_dup (responce);
            zmsg_add (responce_dup, frame);


            vertex_t *vertex;
            vertex_init (&vertex);
            memcpy (vertex, zlist_first (on_give->unc_vertices),
                    sizeof (vertex_t));
            zmsg_add (responce_dup, zframe_new (vertex, sizeof (vertex_t)));

            int i;
            for (i = (diff - 1) * COUNTER_SIZE; i < diff * COUNTER_SIZE; i++) {
                vertex_init (&vertex);
                memcpy (vertex, zlist_next (on_give->unc_vertices),
                        sizeof (vertex_t));

                zmsg_add (responce_dup, zframe_new (vertex, sizeof (vertex_t)));


            }
            zmsg_wrap (responce_dup, zframe_dup (address));
            zmsg_send (&responce_dup, balance->router_bl);

        }
    }


    zmsg_destroy (&responce);
    zframe_destroy (&address);
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
             "\n%s:balance\nA NEW_CHUNK has arrived for the on_receive event with id:%d and giving key:%s",
             balance->self_key, on_receive->un_id, on_receive->action->key);

    zmsg_t *responce = zmsg_new ();
    zframe_t *frame =
        zframe_new (balance->self_key, strlen (balance->self_key));
    zmsg_add (responce, frame);
    frame = zframe_new (CONFIRM_CHUNK, 1);
    zmsg_add (responce, frame);
    zmsg_add (responce, id_frame);




    frame = zmsg_first (msg);

    uint64_t counter;
    memcpy (&counter, zframe_data (frame), sizeof (uint64_t));

//update missed_chunkes

    if (counter == 0) {
//EOTransm. signal
//if there are missing chunks request them,otherwise free the event 

        uint64_t total_counter;
        frame = zmsg_next (msg);
        memcpy (&total_counter, zframe_data (frame), sizeof (uint64_t));

//check if there where more chunks than the ones received           
//add them with the rest missing if necessary
        if ((total_counter != on_receive->counter)) {

            uint64_t i;
            for (i = on_receive->counter + 1; i <= total_counter; i++) {

                zlist_append (on_receive->m_counters, &i);

            }

        }





    }
    else {

        if (counter > on_receive->counter + 1) {

            uint64_t i;
            for (i = on_receive->counter + 1; i < counter; i++) {

                zlist_append (on_receive->m_counters, &i);

            }



        }


    }

//If there are missed chunkes, a request for them is done in balance_lazy_pirate
//that is why we update the clock

    on_receive->last_time = zclock_time ();
    balance_update_receive_timer (balance, on_receive);

    if ((counter == 0) && (zlist_size (on_receive->m_counters) == 0)) {

        frame = zframe_new (&counter, sizeof (uint64_t));
        zmsg_add (responce, frame);

        zmsg_wrap (responce, address);
        zmsg_send (&responce, balance->router_bl);

//add the interval with the others
        intervals_add (balance->intervals, on_receive->interval);
        intervals_print (balance->intervals);

        //erase event if it exists
        if (events_update (balance->events, on_receive->action)) {
            free (on_receive->action);
        }
        else {
//add action to the list
            zlist_append (balance->actions, on_receive->action);

        }


        //destroy on_receive
        zlist_remove (balance->on_receives, on_receive);
        on_receive_destroy (&on_receive);



//check whether any of the events can occur due to this action

        event_t *event;
        while ((event = events_possible (balance->events, balance->intervals))) {
//perform this event

//update the intervals

            interval_t *interval;
            interval_init (&interval, &(event->start), &(event->end));
            intervals_remove (balance->intervals, interval);
            intervals_print (balance->intervals);

//if the node is dead, we drop all the vertices that corresponds to this event
            if (!(event->dead)) {
//update un_id;
                if (balance->un_id > 1000000000) {
                    balance->un_id = 1;
                }
                else {
                    balance->un_id++;
                }


//create on_give object
                on_give_t *on_give;
                on_give_init (&on_give, balance, event, balance->un_id);

//update balance object
                balance_update_give_timer (balance, on_give);



//put on_give event into the list
                zlist_append (balance->on_gives, on_give);
            }
            else {
                event_clean (event, balance->hash);
                zlist_remove (balance->events, event);
                balance_sync (balance, event->key);
                free (event);
            }
        }






        zmsg_destroy (&msg);

    }
    else {

        if (counter < on_receive->counter + 1) {
//received duplicate
//drop
            zmsg_destroy (&msg);
        }
        else {
//get the data
            frame = zmsg_next (msg);

            while (frame) {
                int ret;
                uint64_t key;
                memcpy (&key, zframe_data (frame), sizeof (uint64_t));
                khiter_t k = kh_put (vertices, balance->hash, key, &ret);
                assert (ret != 0);
                frame = zmsg_next (msg);
                memcpy (&kh_value (balance->hash, k),
                        zframe_data (frame), sizeof (vertex_t));
                frame = zmsg_next (msg);
            }

            //update last counter
            on_receive->counter++;


//send responce

            frame = zframe_new (&counter, sizeof (uint64_t));
            zmsg_add (responce, frame);

            zmsg_wrap (responce, address);
            zmsg_send (&responce, balance->router_bl);




            zmsg_destroy (&msg);



        }
    }






}


void
balance_new_interval (balance_t * balance, zmsg_t * msg, zframe_t * address)
{

    zframe_t *id_frame = zmsg_pop (msg);
    int id;
    memcpy (&id, zframe_data (id_frame), sizeof (int));
    char key[17] = { 0 };
    memcpy (key, zframe_data (address), zframe_size (address));

    on_receive_t *on_receive =
        on_receives_search (balance->on_receives, id, key);

//send confirmation
    zmsg_t *responce = zmsg_new ();
    zframe_t *frame =
        zframe_new (balance->self_key, strlen (balance->self_key));
    zmsg_add (responce, frame);
    frame = zframe_new (INTERVAL_RECEIVED, 1);
    zmsg_add (responce, frame);
    zmsg_add (responce, id_frame);
    zmsg_wrap (responce, address);
    zmsg_send (&responce, balance->router_bl);





    if (on_receive) {
//duplicate new_interval
        zmsg_destroy (&msg);
    }
    else {


        on_receive_t *on_receive;
        on_receive_init (&on_receive, id, key, msg);
        zlist_append (balance->on_receives, on_receive);


        fprintf (stderr,
                 "\n%s:balance\nA NEW_INTERVAL has arrived for the on_receive event with id:%d and giving key:%s",
                 balance->self_key, on_receive->un_id, on_receive->action->key);


    }

}




void
balance_new_msg (balance_t * balance, zmsg_t * msg)
{

    //destroying the zero frame
    zframe_t *zero = zmsg_pop (msg);
    zframe_destroy (&zero);
    zframe_t *address = zmsg_pop (msg);
    zframe_t *type_fr = zmsg_pop (msg);

    if (memcmp (INTERVAL_RECEIVED, zframe_data (type_fr), 1) == 0) {
        balance_interval_received (balance, msg, address);
    }
    else {
        if (memcmp (CONFIRM_CHUNK, zframe_data (type_fr), 1) == 0) {
            balance_confirm_chunk (balance, msg, address);
        }
        else {
            if (memcmp (MISSED_CHUNKES, zframe_data (type_fr), 1) == 0) {
                balance_missed_chunkes (balance, msg, address);
            }
            else {
                if (memcmp (NEW_CHUNK, zframe_data (type_fr), 1) == 0) {
                    balance_new_chunk (balance, msg, address);
                }
                else {
                    if (memcmp (NEW_INTERVAL, zframe_data (type_fr), 1) == 0) {
                        balance_new_interval (balance, msg, address);
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
balance_lazy_pirate (balance_t * balance)
{

    int64_t time = zclock_time ();
    balance_clear_timer (balance);

//on_receives
    fprintf (stderr, "\n%s:balance:Inside lazy pirate\n", balance->self_key);

    on_receive_t *iter = zlist_first (balance->on_receives);

    while (iter) {
        if (zlist_size (iter->m_counters) != 0) {
            if (time - iter->last_time > ON_TIMEOUT) {
//request missing chunkes
                fprintf (stderr,
                         "\n%s:balance:Requesting missing chunkes from worker %s\n for action  with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key, iter->action->key,
                         iter->action->start.prefix, iter->action->start.suffix,
                         iter->action->end.prefix, iter->action->end.suffix);


                zmsg_t *responce = zmsg_new ();
                zframe_t *frame =
                    zframe_new (balance->self_key, strlen (balance->self_key));
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
                    zframe_new (iter->action->key, strlen (iter->action->key));
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
                     "\n%s:balance: Sending NEW_INTERVAL msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                     balance->self_key, siter->event->key,
                     siter->event->start.prefix, siter->event->start.suffix,
                     siter->event->end.prefix, siter->event->end.suffix);

            zmsg_t *msg = zmsg_new ();
            zframe_t *frame =
                zframe_new (balance->self_key, strlen (balance->self_key));
            zmsg_add (msg, frame);
            frame = zframe_new (NEW_INTERVAL, 1);
            zmsg_add (msg, frame);
            frame = zframe_new (&(siter->un_id), sizeof (int));
            zmsg_add (msg, frame);
            frame =
                zframe_new (&(siter->event->start), sizeof (struct _hkey_t));
            zmsg_add (msg, frame);
            frame = zframe_new (&(siter->event->end), sizeof (struct _hkey_t));
            zmsg_add (msg, frame);


            frame = zframe_new (siter->event->key, strlen (siter->event->key));
            zmsg_wrap (msg, frame);

            zmsg_send (&msg, balance->router_bl);


        }
        else {
            if (siter->state == 1) {

                fprintf (stderr,
                         "\n%s:balance:Sending EOT msg to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key, siter->event->key,
                         siter->event->start.prefix, siter->event->start.suffix,
                         siter->event->end.prefix, siter->event->end.suffix);


                zmsg_t *msg = zmsg_new ();
                zframe_t *frame =
                    zframe_new (balance->self_key, strlen (balance->self_key));
                zmsg_add (msg, frame);
                frame = zframe_new (NEW_CHUNK, 1);
                zmsg_add (msg, frame);
                frame = zframe_new (&(siter->un_id), sizeof (int));
                zmsg_add (msg, frame);
                uint64_t counter = 0;
                frame = zframe_new (&counter, sizeof (uint64_t));
                zmsg_add (msg, frame);
                frame =
                    zframe_new (&(siter->event->key),
                                strlen (siter->event->key));

                zmsg_wrap (msg, frame);

                zmsg_send (&msg, balance->router_bl);



            }
            else {

                fprintf (stderr,
                         "\n%s:balance:Sending next_chunk to worker %s\n for event with\nstart: %lu %lu \n end: %lu %lu",
                         balance->self_key, siter->event->key,
                         siter->event->start.prefix, siter->event->start.suffix,
                         siter->event->end.prefix, siter->event->end.suffix);

                zframe_t *address = zframe_new (&(siter->event->key),
                                                strlen (siter->event->key));

                balance_send_next_chunk (balance, siter, address);


            }
        }

        fprintf (stderr, "\n%s:balance:Updating on_give timer",
                 balance->self_key);
        siter->last_time = time;
        balance_update_give_timer (balance, siter);
        siter = zlist_next (balance->on_gives);


    }
}

void
balance_sync (balance_t * balance, char *key)
{

//only set a synchronization signal if all the give events have finished
    int sync = 1;

    event_t *event = zlist_first (balance->events);

    while (event) {

        if ((strcmp (event->key, key) == 0) && (event->give == 1)) {

//do the sync
            sync = 0;
            break;
        }
        event = zlist_next (balance->events);
    }

    if (sync) {
        char octopus[8];
        oconfig_octopus (balance->worker->config, octopus);

        char *pointer;
        int size;

        char my_res_name[8] = { 0 };
        char my_comp_name[8] = { 0 };

        part_path (balance->self_key, 1, pointer, &size);
        memcpy (my_res_name, pointer, size);
        memcpy (my_comp_name, balance->self_key,
                strlen (balance->self_key) - size - 1);

        char path[1000];

        sprintf (path, "/%s/computers/%s/worker_nodes/%s/sync", octopus,
                 my_comp_name, my_res_name);
        int result;

        char str_array[16000] = { 0 };  //maximum 1000 deaths 
        int str_array_length = 16000;
        Stat stat;

        result =
            zoo_get (balance->worker->id, path, 0, str_array, &str_array_length,
                     &stat);

        assert (result == ZOK);

        if ((str_array_length == 16000) || (str_array_length == -1)) {

            str_array = {
            0};
            memcpy (string_array, key, 16);

        }
        else {
            memcpy (string_array + str_array_length, key, 16);
        }

        result =
            zoo_set (balance->worker->zh, path, str_array,
                     str_array_length + 16, stat.version);


        assert (ZOK == result);






    }
}
