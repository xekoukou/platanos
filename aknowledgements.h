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








#ifndef OCTOPUS_AKNOWLEDGEMENTS_H_
#define OCTOPUS_AKNOWLEDGEMENTS_H_


#include <stdint.h>
#include <stddef.h>
#include "tree/tree.h"
#include"router.h"
#include"czmq.h"
#include"MurmurHash/MurmurHash3.h"

//TODO We dont need an rb tree here
//but it works anyway

//intervals are disjoint by default
//we sort them by the end

//an interval should never have end and start equal
struct interval_t
{


    struct _hkey_t start;
    struct _hkey_t end;
      RB_ENTRY (interval_t) field;

};

RB_HEAD (intervals_t, interval_t);
RB_PROTOTYPE (intervals_t, interval_t, field, cmp_interval_t);

//the key is the key of the node that will take part in the event /action
//there is only the exception for the first node that will go online
//its first event will have NULL as key

struct event_t
{
    struct _hkey_t start;
    struct _hkey_t end;
    int give;
    char key[100];
};
struct action_t
{				//actions are only vertices that are received
    struct _hkey_t start;
    struct _hkey_t end;
    char key[100];
};


typedef struct event_t event_t;
typedef struct action_t action_t;


typedef struct interval_t interval_t;
typedef struct intervals_t intervals_t;

void intervals_init (intervals_t ** intervals);
void
interval_init (interval_t ** interval, struct _hkey_t *start,
	       struct _hkey_t *end);

//the msg is not deleted, it will be used to create action_t later
void interval_minit (interval_t ** interval, zmsg_t * msg);

//th interval is closed
int interval_belongs_h (interval_t * interval, struct _hkey_t *hkey);

int interval_belongs (interval_t * interval, uint64_t key);

void intervals_add (intervals_t * intervals, interval_t * interval);

//returns the integral if it is contained inside one integral or NULL
interval_t *intervals_contained (intervals_t * intervals,
				 interval_t * interval);

//removes and free interval
//returns true if there was a change
int intervals_remove (intervals_t * intervals, interval_t * interval);

int intervals_belongs_h (intervals_t * intervals, struct _hkey_t *hkey);

//return true if it belongs to one of the intervals
int intervals_belongs (intervals_t * intervals, uint64_t key);

void intervals_print (intervals_t *intervals);


void event_init (event_t ** event, struct _hkey_t start, struct _hkey_t end,
		 int give, char *key);

event_t *events_search (zlist_t * events, action_t * action);

//this is used when a node dies and we need to clean the previous events
void events_remove (zlist_t * events, node_t * node);

//returns 1 if there was an event that was erased by this action
int events_update (zlist_t * events, action_t * action);




//returns the first event that can occur with the current nodes that we possess or NULL
event_t *events_possible (zlist_t * events, intervals_t * intevals);



int event_possible (event_t * event, intervals_t * intervals);

action_t *actions_search (zlist_t * actions, event_t * event);

//returns 1 if there was an action that was erased by this event
int actions_update (zlist_t * actions, event_t * event);

//action is only created by a received msg
void action_minit (action_t ** action, zmsg_t * msg);

#endif
