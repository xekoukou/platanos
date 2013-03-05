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


#ifndef _OCTOPUS_ON_GIVE_H_
#define _OCTOPUS_ON_GIVE_H_

#include"event.h"
#include"node.h"
#include"vertex.h"



typedef struct
{
    event_t *event;             //the event that is happening, must also be removed from events list before freed
    zlist_t *unc_vertices;      //vertices that have been send but not confirmed
    // contains the khiter where it is saved by the khash

    uint64_t rec_counter;       //the counter of the last confirmation
    int un_id;
    uint64_t last_time;         //last_time till I sent the interval(initial msg) or last time till
//i sent the ending msg(0 counter)
//those operations will be repeated until they succeed
    int state;                  //0 for interval send
    //1 for ending msg
//last counter is used for the last chunk plus to see if I have already received an Interval_received before
//due to possible duplicates (lazy pirate)
//must be set to zero
    uint64_t last_counter;

} on_give_t;                    //ongoing event

void on_give_init (on_give_t ** on_give, event_t * event, int un_id);

//destroy this after you have removed the event from the events list
//this will free the event
//also remove it from the on_gives list
//also all the vertices need to be freed before destroying this

void on_give_destroy (on_give_t * on_give);

//used when we receive a remove_node event
void on_gives_remove (zlist_t * on_gives, zlist_t * events, node_t * node);

//linear search
on_give_t *on_gives_search_id (zlist_t * on_gives, int id);

#endif
