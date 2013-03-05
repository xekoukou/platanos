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


#ifndef OCTOPUS_ACTION_H_
#define OCTOPUS_ACTION_H_

#include<czmq.h>
#include "hkey.h"

//the key is the key of the node that will take part in the event /action
//there is only the exception for the first node that will go online
//its first event will have NULL as key
//TODO ????

struct action_t
{                               //actions are only vertices that are received
    struct _hkey_t start;
    struct _hkey_t end;
    char key[7];
};

typedef struct action_t action_t;

#include "event.h"

//action is only created by a received msg
void action_minit (action_t ** action, char *key, zmsg_t * msg);





#endif
