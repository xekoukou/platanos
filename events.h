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


#ifndef OCTOPUS_EVENTS_H_
#define OCTOPUS_EVENTS_H_

#include "event.h"
#include "action.h"
#include "router.h"

event_t *events_search (zlist_t * events, action_t * action);

//this is used when a node dies and we need to clean the previous events

event_t *events_remove (zlist_t * events, node_t * node);

//returns 1 if there was an event that was erased by this action
int events_update (zlist_t * events, action_t * action);




//returns the first event that can occur with the current nodes that we possess or NULL
event_t *events_possible (zlist_t * events, intervals_t * intevals);




#endif
