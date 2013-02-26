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


#ifndef OCTOPUS_ACTIONS_H_
#define OCTOPUS_ACTIONS_H_

#include "action.h"
#include<czmq.h>
#include"event.h"
#include"intervals.h"

action_t *actions_search (zlist_t * actions, event_t * event);

//returns 1 if there was an action that was erased by this event
int actions_update (zlist_t * actions, event_t * event);



#endif
